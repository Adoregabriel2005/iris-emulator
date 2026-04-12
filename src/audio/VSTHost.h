#pragma once

#include <QString>
#include <QStringList>
#include <functional>
#include <memory>
#include <vector>
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// VSTPlugin — wraps a single loaded VST3 plugin instance
// ─────────────────────────────────────────────────────────────────────────────
struct VSTPluginInfo {
    QString path;       // absolute path to .vst3 bundle/dll
    QString name;       // plugin display name
    QString vendor;
    QString category;
    bool    enabled = true;
};

class VSTPlugin
{
public:
    explicit VSTPlugin(const VSTPluginInfo& info);
    ~VSTPlugin();

    bool load(double sampleRate, int maxBlockSize);
    void unload();
    bool isLoaded() const { return m_loaded; }

    // Process interleaved stereo float32 in-place
    void process(float* left, float* right, int numFrames);

    const VSTPluginInfo& info() const { return m_info; }
    VSTPluginInfo& info() { return m_info; }

    // Parameter access (simplified — index-based)
    int   paramCount() const;
    float getParam(int index) const;
    void  setParam(int index, float value);
    QString paramName(int index) const;

    // Open native editor window. parentHandle = HWND on Windows.
    bool openEditor(void* parentHandle = nullptr);

private:
    VSTPluginInfo m_info;
    bool          m_loaded = false;

    // VST3 internals — forward declared to avoid pulling in VST3 SDK headers here
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

// ─────────────────────────────────────────────────────────────────────────────
// VSTHost — manages the plugin chain and the audio post-proc callback
// ─────────────────────────────────────────────────────────────────────────────
class VSTHost
{
public:
    static VSTHost& instance();

    // Called once when Jaguar audio starts
    void init(double sampleRate, int maxBlockSize);
    void shutdown();

    // The callback registered with DACSetPostProcCallback
    // Converts S16 stereo → float → VST chain → float → S16
    static void audioPostProc(int16_t* samples, int numStereoFrames);

    // Plugin management
    bool        addPlugin(const QString& path);
    void        removePlugin(int index);
    void        movePlugin(int from, int to);
    void        setPluginEnabled(int index, bool enabled);

    int                         pluginCount() const;
    const VSTPluginInfo&        pluginInfo(int index) const;
    VSTPlugin*                  plugin(int index);

    // Scan a directory for .vst3 files
    QStringList scanDirectory(const QString& dir) const;

    // Persist/restore chain
    void saveChain(const QString& settingsKey = "VST/Chain") const;
    void loadChain(const QString& settingsKey = "VST/Chain");

    // Install SDL audio callback wrapper so audioPostProc runs after VJ DAC fills buffer
    // Call this AFTER JaguarInit() + DACInit() have opened SDL audio
    void installSDLCallbackWrapper();

    // Open the plugin's native editor window (IEditController + IPlugView)
    // Returns true if the editor was opened successfully
    bool openEditor(int index, void* parentWindowHandle = nullptr);

    // Global chain on/off — when false, audioPostProc is a no-op
    void setChainEnabled(bool enabled) { m_chainEnabled = enabled; }
    bool isChainEnabled() const        { return m_chainEnabled; }

    // Master volume/gain applied after VST chain (0.0 – 2.0)
    void  setMasterGain(float gain) { m_masterGain = gain; }
    float masterGain() const        { return m_masterGain; }

private:
    VSTHost() = default;

    void processChain(float* left, float* right, int numFrames);

    std::vector<std::unique_ptr<VSTPlugin>> m_plugins;
    double m_sampleRate   = 48000.0;
    int    m_maxBlockSize = 2048;
    float  m_masterGain   = 1.0f;
    bool   m_chainEnabled = true;
    bool   m_initialized  = false;

    // Scratch buffers (avoid alloc in audio thread)
    std::vector<float> m_bufL, m_bufR;
};
