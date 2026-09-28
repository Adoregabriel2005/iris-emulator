#include "JaguarSystem.h"
#include "jaguar.h"
#include "tom.h"
#include "memory.h"
#include "settings.h"
#include "joystick.h"
#include "file.h"
#include "modelsBIOS.h"
#include "log.h"
#include "JaguarAudio.h"
#include "SDLInput.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <cstring>
#include <algorithm>


extern bool        frameDone;

JaguarSystem::JaguarSystem(QObject *parent)
    : IEmulatorCore(parent)
    , m_framebuffer(kTexW * kTexH, 0)
{
    m_frame = QImage(326, 240, QImage::Format_RGB32);
    m_frame.fill(Qt::black);
    QString logPath = QCoreApplication::applicationDirPath() + "/jaguar_iris.log";
    LogInit(logPath.toLocal8Bit().data());
}

JaguarSystem::~JaguarSystem()
{
    stop();
    closeAudio();
    if (m_initialized) {
        JaguarDone();
        m_initialized = false;
    }
    LogDone();
}

bool JaguarSystem::loadROM(const QString &path)
{
    stop();
    closeAudio();
    if (m_initialized) {
        JaguarDone();
        m_initialized = false;
    }

    QSettings qs;
    memset(&vjs, 0, sizeof(vjs));
    vjs.hardwareTypeNTSC        = (qs.value("Jaguar/TVStandard", "NTSC").toString() != "PAL");
    vjs.GPUEnabled              = qs.value("Jaguar/GPUEnabled", true).toBool();
    vjs.DSPEnabled              = qs.value("Jaguar/DSPEnabled", true).toBool();
    vjs.usePipelinedDSP         = qs.value("Jaguar/PipelinedDSP", false).toBool();
    vjs.audioEnabled            = qs.value("Audio/Enabled", true).toBool();
    vjs.useJaguarBIOS           = qs.value("Jaguar/UseBIOS", false).toBool();
    vjs.useRetailBIOS           = qs.value("Jaguar/RetailBIOS", false).toBool();
    vjs.useFastBlitter          = qs.value("Jaguar/FastBlitter", false).toBool();
    vjs.DRAM_size               = 0x200000;
    vjs.jaguarModel             = JAG_M_SERIES;
    vjs.biosType                = BT_M_SERIES;
    vjs.allowM68KExceptionCatch = false;
    vjs.allowWritesToROM        = false;

    {
        QString ep = qs.value("Jaguar/EEPROMPath", "eeproms").toString();
        QDir().mkpath(ep);
        strncpy(vjs.EEPROMPath, ep.toLocal8Bit().constData(), MAX_PATH - 1);
    }

    jaguarCartInserted = true;
    JaguarInit();
    m_initialized = true;

    JaguarSetScreenBuffer(m_framebuffer.data());
    JaguarSetScreenPitch(kTexW);

    qDebug() << "JaguarSystem: loading ROM" << path;
    QByteArray pathBytes = path.toLocal8Bit();
    try {
        qDebug() << "JaguarSystem: calling JaguarLoadFile...";
        if (!JaguarLoadFile(pathBytes.data())) {
            qWarning() << "JaguarSystem: failed to load" << path;
            JaguarDone();
            m_initialized = false;
            return false;
        }
        qDebug() << "JaguarSystem: ROM loaded successfully. CRC32:" << Qt::hex << jaguarMainROMCRC32;
    } catch (...) {
        qCritical() << "JaguarSystem: exception loading" << path;
        JaguarDone();
        m_initialized = false;
        return false;
    }
    qDebug() << "JaguarSystem: ROM loaded, CRC32:" << Qt::hex << jaguarMainROMCRC32;

    // The former filename-based patches sampled PC once per frame. That is not
    // an instruction breakpoint and must not rewrite a game's PC or RAM.
    SelectBIOS(vjs.biosType);
    SET32(jaguarMainRAM, 0, vjs.DRAM_size);
    std::fill(m_framebuffer.begin(), m_framebuffer.end(), 0);

    qDebug() << "JaguarSystem: resetting core...";
    JaguarReset();
    qDebug() << "JaguarSystem: core reset complete";
    memset(m_joypad0, 0, sizeof(m_joypad0));
    memset(m_joypad1, 0, sizeof(m_joypad1));
    m_frame.fill(Qt::black);

    qDebug() << "JaguarSystem: loaded" << QFileInfo(path).fileName()
             << "CRC32=" << Qt::hex << jaguarMainROMCRC32
             << "runAddr=" << Qt::hex << jaguarRunAddress;
    return true;
}

void JaguarSystem::start()
{
    m_running = m_initialized;
    if (m_audioDevice) SDL_PauseAudioDevice(m_audioDevice, !m_running || !m_audioEnabled);
}

void JaguarSystem::stop()
{
    m_running = false;
    if (m_audioDevice) {
        SDL_PauseAudioDevice(m_audioDevice, 1);
        SDL_ClearQueuedAudio(m_audioDevice);
    }
}

void JaguarSystem::step()
{
    if (!m_initialized || !m_running) return;
    memcpy(joypad0Buttons, m_joypad0, sizeof(m_joypad0));
    memcpy(joypad1Buttons, m_joypad1, sizeof(m_joypad1));
    JaguarExecuteNew();
    if (!frameDone) {
        qWarning() << "Jaguar: frame did not complete; stopping emulation";
        stop();
        return;
    }

    auto samples = JaguarTakeAudioSamples();
    if (m_audioDevice && m_audioEnabled && !samples.empty()) {
        for (auto &sample : samples) sample = static_cast<int16_t>(sample * m_audioVolume / 100);
        // Limit latency in uncapped mode. This affects host playback only.
        if (SDL_GetQueuedAudioSize(m_audioDevice) > 48000 * 4 / 5)
            SDL_ClearQueuedAudio(m_audioDevice);
        SDL_QueueAudio(m_audioDevice, samples.data(), static_cast<Uint32>(samples.size() * sizeof(int16_t)));
    }

    // Use exactly the same virtual viewport as TOM's scanline renderer.
    const int visW = std::clamp(static_cast<int>(TOMGetVideoModeWidth()), 1, kTexW);
    const int fields = (TOMGetVP() & 1) ? 1 : 2;
    const int visH = std::clamp(static_cast<int>(TOMGetVideoModeHeight()) * fields, 1, kTexH);
    if (m_frame.size() != QSize(visW, visH))
        m_frame = QImage(visW, visH, QImage::Format_RGB32);
    for (int y = 0; y < visH; ++y) {
        const uint32_t *src = m_framebuffer.data() + y * kTexW;
        QRgb *dst = reinterpret_cast<QRgb *>(m_frame.scanLine(y));
        for (int x = 0; x < visW; ++x) {
            const uint32_t p = src[x];
            dst[x] = qRgb((p >> 24) & 0xFF, (p >> 16) & 0xFF, (p >> 8) & 0xFF);
        }
    }
}

QImage JaguarSystem::getFrame() const { return m_frame; }

bool JaguarSystem::saveState(const QString &path) { Q_UNUSED(path); return false; }
bool JaguarSystem::loadState(const QString &path) { Q_UNUSED(path); return false; }

void JaguarSystem::setJoystickState(const JoystickState &s)
{
    memset(m_joypad0, 0, sizeof(m_joypad0));
    if (s.up)    m_joypad0[BUTTON_U]     = 1;
    if (s.down)  m_joypad0[BUTTON_D]     = 1;
    if (s.left)  m_joypad0[BUTTON_L]     = 1;
    if (s.right) m_joypad0[BUTTON_R]     = 1;
    if (s.fire)  m_joypad0[BUTTON_B]     = 1;
    if (s.reset) m_joypad0[BUTTON_PAUSE] = 1;
}

void JaguarSystem::setJaguarInputState(const JaguarInputState &s)
{
    memset(m_joypad0, 0, sizeof(m_joypad0));
    if (s.up)     m_joypad0[BUTTON_U]      = 1;
    if (s.down)   m_joypad0[BUTTON_D]      = 1;
    if (s.left)   m_joypad0[BUTTON_L]      = 1;
    if (s.right)  m_joypad0[BUTTON_R]      = 1;
    if (s.a)      m_joypad0[BUTTON_A]      = 1;
    if (s.b)      m_joypad0[BUTTON_B]      = 1;
    if (s.c)      m_joypad0[BUTTON_C]      = 1;
    if (s.option) m_joypad0[BUTTON_OPTION] = 1;
    if (s.pause)  m_joypad0[BUTTON_PAUSE]  = 1;
    if (s.star)   m_joypad0[BUTTON_s]      = 1;
    if (s.hash)   m_joypad0[BUTTON_d]      = 1;
    if (s.n0)  m_joypad0[BUTTON_0] = 1;
    if (s.n1)  m_joypad0[BUTTON_1] = 1;
    if (s.n2)  m_joypad0[BUTTON_2] = 1;
    if (s.n3)  m_joypad0[BUTTON_3] = 1;
    if (s.n4)  m_joypad0[BUTTON_4] = 1;
    if (s.n5)  m_joypad0[BUTTON_5] = 1;
    if (s.n6)  m_joypad0[BUTTON_6] = 1;
    if (s.n7)  m_joypad0[BUTTON_7] = 1;
    if (s.n8)  m_joypad0[BUTTON_8] = 1;
    if (s.n9)  m_joypad0[BUTTON_9] = 1;
}

void JaguarSystem::initAudio(const QString &deviceName)
{
    closeAudio();
    if (!m_initialized) return;
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        qWarning() << "Jaguar audio:" << SDL_GetError();
        return;
    }
    SDL_AudioSpec desired{};
    desired.freq = 48000;
    desired.format = AUDIO_S16SYS;
    desired.channels = 2;
    desired.samples = 1024;
    const QByteArray name = deviceName.toUtf8();
    const char *device = deviceName.isEmpty() || deviceName == "default" ? nullptr : name.constData();
    m_audioDevice = SDL_OpenAudioDevice(device, 0, &desired, nullptr, 0);
    if (!m_audioDevice) qWarning() << "Jaguar audio:" << SDL_GetError();
    else SDL_PauseAudioDevice(m_audioDevice, !m_running || !m_audioEnabled);
}

void JaguarSystem::closeAudio()
{
    if (m_audioDevice) { SDL_CloseAudioDevice(m_audioDevice); m_audioDevice = 0; }
}

void JaguarSystem::setAudioVolume(int percent)
{
    m_audioVolume = std::clamp(percent, 0, 100);
}

void JaguarSystem::setAudioEnabled(bool enabled)
{
    m_audioEnabled = enabled;
    if (m_audioDevice) {
        SDL_ClearQueuedAudio(m_audioDevice);
        SDL_PauseAudioDevice(m_audioDevice, !m_running || !enabled);
    }
}
