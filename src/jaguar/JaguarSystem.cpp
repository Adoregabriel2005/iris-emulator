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
#include <QSaveFile>
#include <QDataStream>
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
    const QString smokeOutput = QCoreApplication::instance()->property("smokeOutput").toString();
    if (!smokeOutput.isEmpty()) logPath = smokeOutput + "/jaguar_iris.log";
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
    m_audioSamples = m_nonzeroSamples = m_clippedSamples = 0;
    m_audioPeak = m_audioQueueErrors = m_audioQueueResets = m_audioUnderruns = 0;
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
    if (m_audioDevice) SDL_PauseAudioDevice(m_audioDevice, !m_running || !m_audioEnabled || !m_audioPrimed);
}

void JaguarSystem::stop()
{
    m_running = false;
    if (m_audioDevice) {
        SDL_PauseAudioDevice(m_audioDevice, 1);
        SDL_ClearQueuedAudio(m_audioDevice);
        m_audioPrimed = false;
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
    const bool captureAudio = QCoreApplication::instance()->property("smokeAudio").toBool();
    for (const auto sample : samples) {
        ++m_audioSamples;
        m_nonzeroSamples += sample != 0;
        m_clippedSamples += sample == INT16_MIN || sample == INT16_MAX;
        m_audioPeak = std::max(m_audioPeak, static_cast<unsigned>(std::abs(static_cast<int>(sample))));
        if (captureAudio && m_audioCapture.size() < 48000 * 4 * 30) {
            m_audioCapture.append(static_cast<char>(sample & 0xFF));
            m_audioCapture.append(static_cast<char>(static_cast<uint16_t>(sample) >> 8));
        }
    }
    if (m_audioDevice && m_audioEnabled && !samples.empty()) {
        for (auto &sample : samples) sample = static_cast<int16_t>(sample * m_audioVolume / 100);
        if (m_audioPrimed && SDL_GetQueuedAudioSize(m_audioDevice) == 0) {
            ++m_audioUnderruns;
            m_audioPrimed = false;
            SDL_PauseAudioDevice(m_audioDevice, 1);
        }
        // Limit latency in uncapped mode. This affects host playback only.
        if (SDL_GetQueuedAudioSize(m_audioDevice) > std::max(Uint32(48000 * 4 / 5), m_audioPrebufferBytes * 3)) {
            SDL_ClearQueuedAudio(m_audioDevice);
            m_audioPrimed = false;
            ++m_audioQueueResets;
        }
        if (SDL_QueueAudio(m_audioDevice, samples.data(), static_cast<Uint32>(samples.size() * sizeof(int16_t))) != 0)
            ++m_audioQueueErrors;
        // Respect the configured latency and re-prime after an underrun.
        if (!m_audioPrimed && SDL_GetQueuedAudioSize(m_audioDevice) >= m_audioPrebufferBytes) {
            m_audioPrimed = true;
            SDL_PauseAudioDevice(m_audioDevice, 0);
        }
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

qint64 JaguarSystem::framePeriodNanoseconds() const
{
    // JaguarExecuteNew completes 525 NTSC or 625 PAL half-lines per call.
    return vjs.hardwareTypeNTSC ? 16683333 : 20000000;
}

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
    m_audioPrebufferBytes = 48000 * 4 * std::clamp(QSettings().value("Audio/Latency", 128).toInt(), 16, 256) / 1000;
    const QByteArray name = deviceName.toUtf8();
    const char *device = deviceName.isEmpty() || deviceName == "default" || deviceName == "System Default"
        ? nullptr : name.constData();
    m_audioDevice = SDL_OpenAudioDevice(device, 0, &desired, nullptr, 0);
    if (!m_audioDevice && device) {
        qWarning() << "Jaguar audio device unavailable:" << deviceName
                   << "- using the default device";
        m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, nullptr, 0);
    }
    if (!m_audioDevice) qWarning() << "Jaguar audio:" << SDL_GetError();
    else {
        m_audioPrimed = false;
        SDL_PauseAudioDevice(m_audioDevice, 1);
        qInfo() << "Jaguar audio opened:" << SDL_GetCurrentAudioDriver() << "48000 Hz stereo";
    }
}

void JaguarSystem::closeAudio()
{
    if (m_audioDevice) { SDL_CloseAudioDevice(m_audioDevice); m_audioDevice = 0; }
    m_audioPrimed = false;
    if (!m_audioCapture.isEmpty()) {
        const QString output = QCoreApplication::instance()->property("smokeOutput").toString();
        QSaveFile wav(output + "/audio.wav");
        if (wav.open(QIODevice::WriteOnly)) {
            QDataStream stream(&wav);
            stream.setByteOrder(QDataStream::LittleEndian);
            stream.writeRawData("RIFF", 4); stream << quint32(36 + m_audioCapture.size());
            stream.writeRawData("WAVEfmt ", 8); stream << quint32(16) << quint16(1) << quint16(2)
                << quint32(48000) << quint32(48000 * 4) << quint16(4) << quint16(16);
            stream.writeRawData("data", 4); stream << quint32(m_audioCapture.size());
            stream.writeRawData(m_audioCapture.constData(), m_audioCapture.size());
            wav.commit();
        }
        m_audioCapture.clear();
    }
}

QVariantMap JaguarSystem::audioDiagnostics() const
{
    return {{"deviceOpened", m_audioDevice != 0}, {"driver", QString::fromUtf8(SDL_GetCurrentAudioDriver() ? SDL_GetCurrentAudioDriver() : "")},
        {"enabled", m_audioEnabled}, {"sampleRate", 48000}, {"channels", 2},
        {"samples", QVariant::fromValue(m_audioSamples)}, {"nonzeroSamples", QVariant::fromValue(m_nonzeroSamples)},
        {"clippedSamples", QVariant::fromValue(m_clippedSamples)}, {"peak", m_audioPeak},
        {"queueErrors", m_audioQueueErrors}, {"queueResets", m_audioQueueResets}, {"underruns", m_audioUnderruns},
        {"latencyMs", m_audioPrebufferBytes * 1000 / (48000 * 4)},
        {"queuedBytes", m_audioDevice ? SDL_GetQueuedAudioSize(m_audioDevice) : 0}};
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
        m_audioPrimed = false;
        SDL_PauseAudioDevice(m_audioDevice, 1);
    }
}
