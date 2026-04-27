#include "LynxSystem.h"
#include "gearlynx.h"

#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QStringList>
#include <cstring>
#include <algorithm>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

static constexpr int GLYNX_FRAME_BYTES    = GLYNX_SCREEN_WIDTH * GLYNX_SCREEN_HEIGHT * 4;
static constexpr int MAX_AUDIO_SAMPLES    = 65536;

#include "SDLInput.h"

LynxSystem::LynxSystem(QObject *parent)
    : IEmulatorCore(parent)
    , m_core(new GearlynxCore())
    , m_frame_buffer(GLYNX_FRAME_BYTES)
    , m_sample_buffer(MAX_AUDIO_SAMPLES)
{
    m_core->Init(GLYNX_PIXEL_RGBA8888);
    m_frame = QImage(GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT, QImage::Format_RGBA8888);
}
LynxSystem::~LynxSystem()
{
    closeAudio();
    delete m_core;
}

bool LynxSystem::loadROM(const QString &path)
{
    if (!m_core)
        return false;

    qDebug() << "[Lynx] Loading ROM:" << path;

    QSettings settings;
    QString configuredBios = settings.value("Lynx/BiosPath", "").toString().trimmed();
    bool biosLoaded = false;

    // Try configured BIOS path first
    if (!configuredBios.isEmpty()) {
        if (QFile::exists(configuredBios)) {
            auto state = m_core->LoadBios(configuredBios.toUtf8().constData());
            if (state == BIOS_LOAD_OK) {
                biosLoaded = true;
                qDebug() << "[Lynx] BIOS loaded successfully from configured path:" << configuredBios;
            } else {
                qWarning() << "[Lynx] Failed to load BIOS from configured path:" << configuredBios << "- state:" << state;
            }
        } else {
            qWarning() << "[Lynx] Configured BIOS path does not exist:" << configuredBios;
        }
    } else {
        qDebug() << "[Lynx] No BIOS path configured, searching for BIOS automatically...";
    }

    // Search for BIOS in common locations
    if (!biosLoaded) {
        QStringList bootNames = { "lynxboot.img", "LinxBoot.lnx", "lynxboot.lnx", "lynx.rom", "lynxboot.mig" };
        QStringList bootDirs = {
            QCoreApplication::applicationDirPath(),
            QFileInfo(path).absolutePath(),
            QFileInfo(path).absolutePath() + "/..",
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
        };

        for (const auto &dir : bootDirs) {
            if (biosLoaded)
                break;
            for (const auto &name : bootNames) {
                QString bootPath = dir + "/" + name;
                if (!QFile::exists(bootPath))
                    continue;

                qDebug() << "[Lynx] Found BIOS at:" << bootPath;
                auto state = m_core->LoadBios(bootPath.toUtf8().constData());
                if (state == BIOS_LOAD_OK) {
                    biosLoaded = true;
                    qDebug() << "[Lynx] BIOS loaded successfully from:" << bootPath;
                    break;
                } else {
                    qWarning() << "[Lynx] Failed to load BIOS from:" << bootPath << "- state:" << state;
                }
            }
        }
    }

    if (!biosLoaded) {
        qWarning() << "[Lynx] BIOS not found; encrypted/commercial ROMs may fail to boot. Homebrew with HLE magic byte should work.";
    }

    m_bios_found = biosLoaded;

    // Load ROM
    bool ok = m_core->LoadROM(path.toUtf8().constData());
    if (!ok) {
        qWarning() << "[Lynx] Failed to load ROM:" << path;
        return false;
    }

    qDebug() << "[Lynx] ROM loaded successfully:" << path;
    qDebug() << "[Lynx] BIOS status:" << (biosLoaded ? "OK" : "MISSING") << "| ROM path:" << path;

    m_lynx_keys = 0;
    m_frame.fill(Qt::black);
    m_core->Pause(false);
    return true;
}

void LynxSystem::start()
{
    m_running = true;
}

void LynxSystem::stop()
{
    m_running = false;
}

void LynxSystem::step()
{
    if (!m_running || !m_core)
        return;

    int sampleCount = 0;
    m_core->RunToVBlank(m_frame_buffer.data(), m_sample_buffer.data(), &sampleCount, nullptr);

    if (m_audio_enabled && m_audio_device > 0 && sampleCount > 0) {
        // Apply volume in-place before queuing
        if (m_audio_volume < 100) {
            float vol = m_audio_volume / 100.0f;
            for (int i = 0; i < sampleCount; i++)
                m_sample_buffer[i] = static_cast<int16_t>(m_sample_buffer[i] * vol);
        }
        // Keep queue bounded to ~2 frames to avoid growing latency
        constexpr Uint32 MAX_QUEUED = GLYNX_AUDIO_BUFFER_SIZE * 2 * sizeof(int16_t);
        Uint32 queuedSize = SDL_GetQueuedAudioSize(m_audio_device);
        if (queuedSize < MAX_QUEUED)
            SDL_QueueAudio(m_audio_device, m_sample_buffer.data(), sampleCount * sizeof(int16_t));
        else {
            // Audio buffer is full - skip this frame's audio to prevent latency buildup
            qDebug() << "[Lynx] Audio buffer full (" << queuedSize << "bytes), skipping audio this frame";
        }
    }

    // Deep copy frame data to avoid dangling pointer when m_frame_buffer is modified
    m_frame = QImage(m_frame_buffer.data(), GLYNX_SCREEN_WIDTH, GLYNX_SCREEN_HEIGHT, QImage::Format_RGBA8888).copy();
}

QImage LynxSystem::getFrame() const
{
    return m_frame;
}

bool LynxSystem::saveState(const QString &path)
{
    if (!m_core)
        return false;
    return m_core->SaveState(path.toUtf8().constData());
}

bool LynxSystem::loadState(const QString &path)
{
    if (!m_core)
        return false;
    return m_core->LoadState(path.toUtf8().constData());
}

void LynxSystem::setJoystickState(const JoystickState &s)
{
    if (!m_core)
        return;

    // Legacy mapping: 2600 actions -> Lynx keys (kept for backward compatibility)
    auto updateKey = [&](GLYNX_Keys key, bool pressed) {
        if (pressed && !(m_lynx_keys & key))
            m_core->KeyPressed(key);
        else if (!pressed && (m_lynx_keys & key))
            m_core->KeyReleased(key);
    };

    updateKey(GLYNX_KEY_RIGHT, s.right);
    updateKey(GLYNX_KEY_LEFT, s.left);
    updateKey(GLYNX_KEY_DOWN, s.down);
    updateKey(GLYNX_KEY_UP, s.up);
    updateKey(GLYNX_KEY_OPTION1, s.reset);
    updateKey(GLYNX_KEY_OPTION2, s.select);
    updateKey(GLYNX_KEY_B, s.p0DiffToggle);
    updateKey(GLYNX_KEY_A, s.fire);
    updateKey(GLYNX_KEY_PAUSE, s.colorBWToggle);

    m_lynx_keys = 0;
    if (s.right) m_lynx_keys |= GLYNX_KEY_RIGHT;
    if (s.left) m_lynx_keys |= GLYNX_KEY_LEFT;
    if (s.down) m_lynx_keys |= GLYNX_KEY_DOWN;
    if (s.up) m_lynx_keys |= GLYNX_KEY_UP;
    if (s.reset) m_lynx_keys |= GLYNX_KEY_OPTION1;
    if (s.select) m_lynx_keys |= GLYNX_KEY_OPTION2;
    if (s.p0DiffToggle) m_lynx_keys |= GLYNX_KEY_B;
    if (s.fire) m_lynx_keys |= GLYNX_KEY_A;
    if (s.colorBWToggle) m_lynx_keys |= GLYNX_KEY_PAUSE;
}

void LynxSystem::setLynxInputState(const LynxInputState &s)
{
    if (!m_core)
        return;

    // Native Lynx input with correct semantics
    auto updateKey = [&](GLYNX_Keys key, bool pressed) {
        if (pressed && !(m_lynx_keys & key))
            m_core->KeyPressed(key);
        else if (!pressed && (m_lynx_keys & key))
            m_core->KeyReleased(key);
    };

    updateKey(GLYNX_KEY_UP, s.up);
    updateKey(GLYNX_KEY_DOWN, s.down);
    updateKey(GLYNX_KEY_LEFT, s.left);
    updateKey(GLYNX_KEY_RIGHT, s.right);
    updateKey(GLYNX_KEY_A, s.a);
    updateKey(GLYNX_KEY_B, s.b);
    updateKey(GLYNX_KEY_OPTION1, s.leftShoulder);  // OPTION1 = Left Shoulder
    updateKey(GLYNX_KEY_OPTION2, s.rightShoulder); // OPTION2 = Right Shoulder
    updateKey(GLYNX_KEY_PAUSE, s.start);           // PAUSE = Start

    m_lynx_keys = 0;
    if (s.up) m_lynx_keys |= GLYNX_KEY_UP;
    if (s.down) m_lynx_keys |= GLYNX_KEY_DOWN;
    if (s.left) m_lynx_keys |= GLYNX_KEY_LEFT;
    if (s.right) m_lynx_keys |= GLYNX_KEY_RIGHT;
    if (s.a) m_lynx_keys |= GLYNX_KEY_A;
    if (s.b) m_lynx_keys |= GLYNX_KEY_B;
    if (s.leftShoulder) m_lynx_keys |= GLYNX_KEY_OPTION1;
    if (s.rightShoulder) m_lynx_keys |= GLYNX_KEY_OPTION2;
    if (s.start) m_lynx_keys |= GLYNX_KEY_PAUSE;
}


LynxSystem::DebugInfo LynxSystem::getDebugInfo() const
{
    DebugInfo info;
    info.isRunning = m_running;

    if (m_core && m_running) {
        // Get CPU state from Gearlynx M6502
        M6502* cpu = m_core->GetM6502();
        if (cpu) {
            M6502::M6502_State* state = cpu->GetState();
            if (state) {
                info.pc = state->PC.GetValue();
                info.a = state->A.GetValue();
                info.x = state->X.GetValue();
                info.y = state->Y.GetValue();
                info.sp = state->S.GetValue();
                info.p = state->P.GetValue();
                info.totalCycles = state->total_ticks;
            }
        }
    }

    return info;
}


void LynxSystem::initAudio(const QString &deviceName)
{
    closeAudio();

    qDebug() << "[Lynx] Initializing audio...";

    SDL_AudioSpec want{}, have{};
    want.freq = 44100;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 1024;
    want.callback = nullptr;  // push model — sem callback
    want.userdata = nullptr;

    const char *dev = nullptr;
    QByteArray devUtf8;
    if (!deviceName.isEmpty() && deviceName != "default") {
        devUtf8 = deviceName.toUtf8();
        dev = devUtf8.constData();
        qDebug() << "[Lynx] Using audio device:" << deviceName;
    }

    m_audio_device = SDL_OpenAudioDevice(dev, 0, &want, &have, 0);
    if (m_audio_device > 0) {
        m_audio_sample_rate = have.freq;
        SDL_PauseAudioDevice(m_audio_device, 0);
        qDebug() << "[Lynx] Audio initialized:" << have.freq << "Hz," << have.channels << "channels, device ID:" << m_audio_device;
    } else {
        qWarning() << "[Lynx] SDL audio init failed:" << SDL_GetError();
    }
}

void LynxSystem::closeAudio()
{
    if (m_audio_device > 0) {
        SDL_CloseAudioDevice(m_audio_device);
        m_audio_device = 0;
    }
}

void LynxSystem::setAudioVolume(int percent)
{
    m_audio_volume = qBound(0, percent, 100);
}

void LynxSystem::setAudioEnabled(bool enabled)
{
    m_audio_enabled = enabled;
    if (m_audio_device > 0 && !enabled)
        SDL_ClearQueuedAudio(m_audio_device);
}

