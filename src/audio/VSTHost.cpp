#include "VSTHost.h"

#include <SDL.h>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <algorithm>
#include <cstring>
#include <cmath>

// ─── VST3 SDK ────────────────────────────────────────────────────────────────
#include "vst3sdk/pluginterfaces/base/ipluginbase.h"
#include "vst3sdk/pluginterfaces/vst/ivstcomponent.h"
#include "vst3sdk/pluginterfaces/vst/ivstaudioprocessor.h"
#include "vst3sdk/pluginterfaces/vst/ivsteditcontroller.h"
#include "vst3sdk/pluginterfaces/gui/iplugview.h"

using namespace Steinberg;
using namespace Steinberg::Vst;

// ─── Platform DLL ────────────────────────────────────────────────────────────
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
   typedef HMODULE DylibHandle;
#  define DYLIB_OPEN(p)   LoadLibraryW(reinterpret_cast<LPCWSTR>((p).utf16()))
#  define DYLIB_SYM(h,s)  GetProcAddress((h),(s))
#  define DYLIB_CLOSE(h)  FreeLibrary(h)
#else
#  include <dlfcn.h>
   typedef void* DylibHandle;
#  define DYLIB_OPEN(p)   dlopen((p).toUtf8().constData(), RTLD_LAZY)
#  define DYLIB_SYM(h,s)  dlsym((h),(s))
#  define DYLIB_CLOSE(h)  dlclose(h)
#endif

typedef IPluginFactory* (PLUGIN_API *GetFactoryProc)();

// ─── Minimal host context ────────────────────────────────────────────────────
class IrisHostContext : public FUnknown {
public:
    tresult PLUGIN_API queryInterface(const TUID, void** obj) override { *obj=nullptr; return kNoInterface; }
    uint32  PLUGIN_API addRef()  override { return 1; }
    uint32  PLUGIN_API release() override { return 1; }
};
static IrisHostContext gHostContext;

// ─── VSTPlugin::Impl ─────────────────────────────────────────────────────────
struct VSTPlugin::Impl {
    DylibHandle      handle    = nullptr;
    IPluginFactory*  factory   = nullptr;
    IComponent*      component = nullptr;
    IAudioProcessor* processor = nullptr;
    float* chL = nullptr;
    float* chR = nullptr;
    int    bufSz = 0;
    bool   active = false;

    void ensureBuf(int n) {
        if (n > bufSz) {
            delete[] chL; delete[] chR;
            chL = new float[n](); chR = new float[n]();
            bufSz = n;
        }
    }
    ~Impl() {
        if (processor)  { processor->setProcessing(false); processor->release(); }
        if (component)  { component->setActive(false); component->terminate(); component->release(); }
        if (factory)    { factory->release(); }
        delete[] chL; delete[] chR;
        if (handle) DYLIB_CLOSE(handle);
    }
};

// ─── VSTPlugin ───────────────────────────────────────────────────────────────
VSTPlugin::VSTPlugin(const VSTPluginInfo& info)
    : m_info(info), m_impl(std::make_unique<Impl>()) {}

VSTPlugin::~VSTPlugin() { unload(); }

bool VSTPlugin::load(double sampleRate, int maxBlockSize)
{
    if (m_loaded) return true;

    QString dllPath = m_info.path;
    QFileInfo fi(dllPath);
    if (fi.isDir()) {
        QString inner = dllPath + "/Contents/x86_64-win/" + fi.baseName() + ".vst3";
        if (!QFileInfo::exists(inner))
            inner = dllPath + "/Contents/x86_64-win/" + fi.fileName();
        if (QFileInfo::exists(inner)) dllPath = inner;
    }

    m_impl->handle = DYLIB_OPEN(dllPath);
    if (!m_impl->handle) { qWarning() << "VSTHost: cannot load" << dllPath; return false; }

    auto initDll = reinterpret_cast<bool(*)()>(DYLIB_SYM(m_impl->handle, "InitDll"));
    if (initDll) initDll();

    auto getFactory = reinterpret_cast<GetFactoryProc>(DYLIB_SYM(m_impl->handle, "GetPluginFactory"));
    if (!getFactory) {
        qWarning() << "VSTHost: no GetPluginFactory in" << dllPath;
        DYLIB_CLOSE(m_impl->handle); m_impl->handle = nullptr; return false;
    }

    m_impl->factory = getFactory();
    if (!m_impl->factory) { DYLIB_CLOSE(m_impl->handle); m_impl->handle = nullptr; return false; }

    // Find first Audio Effect class
    TUID processorCID{}; bool found = false;
    for (int32 i = 0; i < m_impl->factory->countClasses() && !found; i++) {
        PClassInfo ci;
        if (m_impl->factory->getClassInfo(i, &ci) != kResultOk) continue;
        if (strcmp(ci.category, kVstAudioEffectClass) == 0) {
            memcpy(processorCID, ci.cid, sizeof(TUID));
            m_info.name = QString::fromLatin1(ci.name);
            found = true;
        }
    }
    if (!found) {
        qWarning() << "VSTHost: no Audio Effect in" << dllPath;
        m_impl->factory->release(); m_impl->factory = nullptr;
        DYLIB_CLOSE(m_impl->handle); m_impl->handle = nullptr; return false;
    }

    // IComponent
    void* compPtr = nullptr;
    if (m_impl->factory->createInstance(processorCID, IComponent::iid, &compPtr) != kResultOk || !compPtr) {
        qWarning() << "VSTHost: createInstance failed for" << m_info.name;
        m_impl->factory->release(); m_impl->factory = nullptr;
        DYLIB_CLOSE(m_impl->handle); m_impl->handle = nullptr; return false;
    }
    m_impl->component = static_cast<IComponent*>(compPtr);

    if (m_impl->component->initialize(&gHostContext) != kResultOk) {
        qWarning() << "VSTHost: initialize failed for" << m_info.name;
        m_impl->component->release(); m_impl->component = nullptr;
        m_impl->factory->release();   m_impl->factory   = nullptr;
        DYLIB_CLOSE(m_impl->handle);  m_impl->handle    = nullptr; return false;
    }

    // IAudioProcessor
    void* procPtr = nullptr;
    if (m_impl->component->queryInterface(IAudioProcessor::iid, &procPtr) != kResultOk || !procPtr) {
        qWarning() << "VSTHost: IAudioProcessor QI failed for" << m_info.name;
        m_impl->component->terminate(); m_impl->component->release(); m_impl->component = nullptr;
        m_impl->factory->release();     m_impl->factory   = nullptr;
        DYLIB_CLOSE(m_impl->handle);    m_impl->handle    = nullptr; return false;
    }
    m_impl->processor = static_cast<IAudioProcessor*>(procPtr);

    ProcessSetup setup;
    setup.processMode        = kRealtime;
    setup.symbolicSampleSize = kSample32;
    setup.maxSamplesPerBlock = maxBlockSize;
    setup.sampleRate         = sampleRate;
    m_impl->processor->setupProcessing(setup); // non-fatal if fails

    m_impl->component->activateBus(kAudio, kInput,  0, true);
    m_impl->component->activateBus(kAudio, kOutput, 0, true);
    m_impl->component->setActive(true);
    m_impl->processor->setProcessing(true);
    m_impl->active = true;
    m_impl->ensureBuf(maxBlockSize);
    m_loaded = true;

    qDebug() << "VSTHost: loaded" << m_info.name;
    return true;
}

void VSTPlugin::unload()
{
    if (!m_loaded) return;
    m_impl = std::make_unique<Impl>();
    m_loaded = false;
}

void VSTPlugin::process(float* left, float* right, int numFrames)
{
    if (!m_loaded || !m_impl->active || !m_impl->processor) return;
    m_impl->ensureBuf(numFrames);

    memcpy(m_impl->chL, left,  numFrames * sizeof(float));
    memcpy(m_impl->chR, right, numFrames * sizeof(float));

    float* inCh[2]  = { m_impl->chL, m_impl->chR };
    float* outCh[2] = { left, right };

    AudioBusBuffers inBus;  inBus.numChannels=2;  inBus.silenceFlags=0;  inBus.channelBuffers32=inCh;
    AudioBusBuffers outBus; outBus.numChannels=2; outBus.silenceFlags=0; outBus.channelBuffers32=outCh;

    ProcessData data;
    data.processMode        = kRealtime;
    data.symbolicSampleSize = kSample32;
    data.numSamples         = numFrames;
    data.numInputs          = 1;
    data.numOutputs         = 1;
    data.inputs             = &inBus;
    data.outputs            = &outBus;
    data.inputParameterChanges  = nullptr;
    data.outputParameterChanges = nullptr;
    data.inputEvents            = nullptr;
    data.outputEvents           = nullptr;
    data.processContext         = nullptr;

    m_impl->processor->process(data);
}

int     VSTPlugin::paramCount() const       { return 0; }
float   VSTPlugin::getParam(int) const      { return 0.f; }
void    VSTPlugin::setParam(int, float)     {}
QString VSTPlugin::paramName(int) const     { return {}; }

bool VSTPlugin::openEditor(void* /*parentHandle*/)
{
    if (!m_loaded || !m_impl->component) return false;

    void* ctrlPtr = nullptr;
    if (m_impl->component->queryInterface(IEditController::iid, &ctrlPtr) != kResultOk || !ctrlPtr)
        return false;

    auto* ctrl = static_cast<IEditController*>(ctrlPtr);
    IPlugView* view = ctrl->createView(ViewType::kEditor);
    ctrl->release();
    if (!view) return false;

#ifdef _WIN32
    if (view->isPlatformTypeSupported(kPlatformTypeHWND) != kResultOk) {
        view->release(); return false;
    }

    // Register a simple host window class (once)
    static bool s_classRegistered = false;
    static const wchar_t* kWndClass = L"IrisVSTHost";
    if (!s_classRegistered) {
        WNDCLASSEXW wc{};
        wc.cbSize        = sizeof(wc);
        wc.style         = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc   = DefWindowProcW;
        wc.hInstance     = GetModuleHandle(nullptr);
        wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
        wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
        wc.lpszClassName = kWndClass;
        RegisterClassExW(&wc);
        s_classRegistered = true;
    }

    ViewRect rect{};
    view->getSize(&rect);
    int w = (rect.right  > rect.left) ? (rect.right  - rect.left) : 640;
    int h = (rect.bottom > rect.top)  ? (rect.bottom - rect.top)  : 480;

    // Account for window chrome
    RECT wr = { 0, 0, w, h };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExW(0, kWndClass,
        reinterpret_cast<LPCWSTR>(m_info.name.utf16()),
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        wr.right - wr.left, wr.bottom - wr.top,
        nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
    if (!hwnd) { view->release(); return false; }

    if (view->attached(hwnd, kPlatformTypeHWND) != kResultOk) {
        DestroyWindow(hwnd); view->release(); return false;
    }
    // view and hwnd are kept alive — plugin owns the lifecycle
    return true;
#else
    view->release(); return false;
#endif
}

// ─── VSTHost ─────────────────────────────────────────────────────────────────
VSTHost& VSTHost::instance() { static VSTHost s; return s; }

void VSTHost::init(double sampleRate, int maxBlockSize)
{
    m_sampleRate   = sampleRate;
    m_maxBlockSize = maxBlockSize;
    m_bufL.resize(maxBlockSize);
    m_bufR.resize(maxBlockSize);
    m_initialized = true;
    // NOTE: loadChain() is NOT called here automatically.
    // Plugins are only loaded when the user explicitly adds them via Settings.
    QSettings s;
    m_masterGain = s.value("VST/Chain/MasterGain", 1.0f).toFloat();
    qDebug() << "VSTHost: initialized at" << sampleRate << "Hz, block" << maxBlockSize;
}

void VSTHost::shutdown()
{
    // Do NOT auto-save here — chain is only saved when user clicks OK in Settings.
    m_plugins.clear();
    m_initialized = false;
}

void VSTHost::audioPostProc(int16_t* samples, int numStereoFrames)
{
    VSTHost& h = instance();
    if (!h.m_initialized || !h.m_chainEnabled) return;

    const bool hasPlugins = !h.m_plugins.empty();
    const float gain = h.m_masterGain;
    if (!hasPlugins && std::fabs(gain - 1.0f) < 0.001f) return;

    if (numStereoFrames > (int)h.m_bufL.size()) {
        h.m_bufL.resize(numStereoFrames);
        h.m_bufR.resize(numStereoFrames);
    }

    float* L = h.m_bufL.data();
    float* R = h.m_bufR.data();
    const float inv = 1.f / 32768.f;
    for (int i = 0; i < numStereoFrames; i++) {
        L[i] = samples[i*2+0] * inv;
        R[i] = samples[i*2+1] * inv;
    }

    if (hasPlugins) h.processChain(L, R, numStereoFrames);

    for (int i = 0; i < numStereoFrames; i++) {
        float l = std::clamp(L[i] * gain, -1.f, 1.f);
        float r = std::clamp(R[i] * gain, -1.f, 1.f);
        samples[i*2+0] = static_cast<int16_t>(l * 32767.f);
        samples[i*2+1] = static_cast<int16_t>(r * 32767.f);
    }
}

void VSTHost::processChain(float* left, float* right, int numFrames)
{
    for (auto& p : m_plugins)
        if (p && p->isLoaded() && p->info().enabled)
            p->process(left, right, numFrames);
}

// ─── SDL callback wrapper ────────────────────────────────────────────────────
// SDLSoundCallback is static in dac.cpp — we replicate its logic here using
// the exported VJ DSP/event functions (C++ linkage, no extern "C" needed).
// ─────────────────────────────────────────────────────────────────────────────

// VJ DSP/event exports (C++ linkage)
bool   DSPIsRunning();
void   DSPExec(int cycles);
void   HandleNextEvent(int eventType);
void   SetCallbackTime(void (*fn)(), double usecs, int eventType);
double GetTimeToNextEvent(int eventType);

// ltxd/rtxd are C++ references in memory.cpp
extern uint16_t& ltxd;
extern uint16_t& rtxd;

#define IRIS_DAC_RATE    48000
#define IRIS_EVENT_JERRY 1
// RISC clock NTSC
#define IRIS_RISC_CLOCK  26590906

static int    s_bufIdx   = 0;
static int    s_nSamples = 0;
static Uint8* s_sdlBuf   = nullptr;
static bool   s_bufDone  = false;

static void irisI2SCallback()
{
    if (!s_sdlBuf) return;
    reinterpret_cast<uint16_t*>(s_sdlBuf)[s_bufIdx+0] = ltxd;
    reinterpret_cast<uint16_t*>(s_sdlBuf)[s_bufIdx+1] = rtxd;
    s_bufIdx += 2;
    if (s_bufIdx >= s_nSamples) { s_bufDone = true; return; }
    SetCallbackTime(irisI2SCallback, 1000000.0 / IRIS_DAC_RATE, IRIS_EVENT_JERRY);
}

static void irisAudioCallback(void* /*userdata*/, Uint8* stream, int len)
{
    if (!DSPIsRunning()) {
        memset(stream, 0, len);
    } else {
        s_bufIdx   = 0;
        s_sdlBuf   = stream;
        s_nSamples = len / 2;
        s_bufDone  = false;
        SetCallbackTime(irisI2SCallback, 1000000.0 / IRIS_DAC_RATE, IRIS_EVENT_JERRY);
        do {
            double t = GetTimeToNextEvent(IRIS_EVENT_JERRY);
            DSPExec(static_cast<int>(t * IRIS_RISC_CLOCK / 1000000.0));
            HandleNextEvent(IRIS_EVENT_JERRY);
        } while (!s_bufDone);
    }
    VSTHost::audioPostProc(reinterpret_cast<int16_t*>(stream), len / 4);
}

void VSTHost::installSDLCallbackWrapper()
{
    SDL_PauseAudio(1);
    SDL_Delay(5);
    SDL_CloseAudio();

    SDL_AudioSpec desired;
    SDL_zero(desired);
    desired.freq     = IRIS_DAC_RATE;
    desired.format   = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples  = 2048;
    desired.callback = irisAudioCallback;
    desired.userdata = nullptr;

    if (SDL_OpenAudio(&desired, nullptr) < 0) {
        qWarning() << "VSTHost: failed to reopen SDL audio:" << SDL_GetError();
        return;
    }
    SDL_PauseAudio(0);
    qDebug() << "VSTHost: SDL wrapper installed — VST3 chain + master gain active";
}

bool VSTHost::openEditor(int index, void* parentWindowHandle)
{
    VSTPlugin* p = plugin(index);
    if (!p) return false;
    return p->openEditor(parentWindowHandle);
}

// --- Plugin management -------------------------------------------------------
bool VSTHost::addPlugin(const QString& path)
{
    VSTPluginInfo info; info.path=path; info.name=QFileInfo(path).baseName(); info.enabled=true;
    auto p = std::make_unique<VSTPlugin>(info);
    if (!p->load(m_sampleRate, m_maxBlockSize)) return false;
    m_plugins.push_back(std::move(p));
    return true;
}

void VSTHost::removePlugin(int index)
{
    if (index>=0 && index<(int)m_plugins.size())
        m_plugins.erase(m_plugins.begin()+index);
}

void VSTHost::movePlugin(int from, int to)
{
    if (from<0||from>=(int)m_plugins.size()) return;
    if (to<0  ||to  >=(int)m_plugins.size()) return;
    auto tmp = std::move(m_plugins[from]);
    m_plugins.erase(m_plugins.begin()+from);
    m_plugins.insert(m_plugins.begin()+to, std::move(tmp));
}

void VSTHost::setPluginEnabled(int index, bool enabled)
{
    if (index>=0 && index<(int)m_plugins.size())
        m_plugins[index]->info().enabled = enabled;
}

int                  VSTHost::pluginCount() const          { return (int)m_plugins.size(); }
const VSTPluginInfo& VSTHost::pluginInfo(int index) const  { return m_plugins[index]->info(); }
VSTPlugin*           VSTHost::plugin(int index)
{
    if (index>=0 && index<(int)m_plugins.size()) return m_plugins[index].get();
    return nullptr;
}

QStringList VSTHost::scanDirectory(const QString& dir) const
{
    QStringList result;
    QDir d(dir);
    for (const auto& e : d.entryInfoList({"*.vst3"}, QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot))
        result << e.absoluteFilePath();
    return result;
}

void VSTHost::saveChain(const QString& key) const
{
    QSettings s;
    QStringList paths, enabled;
    for (const auto& p : m_plugins) {
        paths   << p->info().path;
        enabled << (p->info().enabled ? "1" : "0");
    }
    s.setValue(key+"/Paths",      paths);
    s.setValue(key+"/Enabled",    enabled);
    s.setValue(key+"/MasterGain", m_masterGain);
}

void VSTHost::loadChain(const QString& key)
{
    QSettings s;
    QStringList paths   = s.value(key+"/Paths").toStringList();
    QStringList enabled = s.value(key+"/Enabled").toStringList();
    m_masterGain = s.value(key+"/MasterGain", 1.0f).toFloat();
    for (int i = 0; i < paths.size(); i++) {
        if (!QFileInfo::exists(paths[i])) continue;
        if (addPlugin(paths[i]) && i < enabled.size())
            m_plugins.back()->info().enabled = (enabled[i]=="1");
    }
}
