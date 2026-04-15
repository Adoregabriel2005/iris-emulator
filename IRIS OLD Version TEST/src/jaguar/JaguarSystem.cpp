#include "JaguarSystem.h"
#include "SDLInput.h"
#include "../audio/VSTHost.h"

// Virtual Jaguar RX core headers
#include "jaguar.h"
#include "tom.h"
#include "jerry.h"
#include "memory.h"
#include "settings.h"
#include "joystick.h"
#include "dac.h"
#include "file.h"
#include "gpu.h"
#include "event.h"
#include "m68000/m68kinterface.h"
#include "jaguar/log.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <QElapsedTimer>
#include <cstring>
#include <algorithm>

// VJ requires these globals — defined in jaguar.cpp and settings.cpp
extern VJSettings vjs;
extern bool startM68KTracing;

JaguarSystem::JaguarSystem(QObject *parent)
    : IEmulatorCore(parent)
    , m_framebuffer(JAG_TEX_WIDTH * JAG_TEX_HEIGHT, 0)
{
    m_frame = QImage(JAG_WIDTH, JAG_HEIGHT, QImage::Format_ARGB32);
    m_frame.fill(Qt::black);

    // Redirect VJ log to a file (required by LogInit signature)
    LogInit("jaguar_iris.log");
}

JaguarSystem::~JaguarSystem()
{
    stop();
    if (m_initialized) {
        VSTHost::instance().shutdown();
        DACDone();
        JaguarDone();
        m_initialized = false;
    }
    LogDone();
}


bool JaguarSystem::loadROM(const QString &path)
{
    if (m_initialized) {
        VSTHost::instance().shutdown();
        DACDone();
        JaguarDone();
        m_initialized = false;
    }

    // Configure VJ settings — read from QSettings
    QSettings qs;
    memset(&vjs, 0, sizeof(vjs));
    vjs.hardwareTypeNTSC  = (qs.value("Jaguar/TVStandard", "NTSC").toString() != "PAL");
    vjs.GPUEnabled        = true; // always enable GPU/OP for correct rendering
    vjs.DSPEnabled        = qs.value("Jaguar/DSPEnabled",  true).toBool();
    vjs.usePipelinedDSP   = false;
    vjs.audioEnabled      = true;
    vjs.useJaguarBIOS     = qs.value("Jaguar/UseBIOS",     false).toBool();
    vjs.useRetailBIOS     = vjs.useJaguarBIOS;
    vjs.useFastBlitter    = qs.value("Jaguar/FastBlitter", false).toBool();
    vjs.DRAM_size         = 0x200000;
    vjs.jaguarModel       = JAG_M_SERIES;
    vjs.biosType          = BT_M_SERIES;
    vjs.allowM68KExceptionCatch = false;
    vjs.allowWritesToROM  = true;  // don't halt on ROM writes

    // EEPROMs path
    {
        QString ep = qs.value("Jaguar/EEPROMPath", "eeproms").toString();
        QDir().mkpath(ep);
        strncpy(vjs.EEPROMPath, ep.toLocal8Bit().constData(), MAX_PATH - 1);
    }

    // BIOS path
    if (vjs.useJaguarBIOS) {
        QString bp = qs.value("Jaguar/BIOSPath", "").toString();
        if (bp.isEmpty()) {
            // Auto-detect next to executable
            QString appDir = QCoreApplication::applicationDirPath();
            for (const QString& name : {"jaguar.j64", "jaguar.rom", "jaguar.bin"}) {
                if (QFileInfo::exists(appDir + "/" + name)) { bp = appDir + "/" + name; break; }
            }
        }
        if (!bp.isEmpty())
            strncpy(vjs.ROMPath, bp.toLocal8Bit().constData(), MAX_PATH - 1);
        else
            vjs.useJaguarBIOS = vjs.useRetailBIOS = false; // BIOS not found, disable
    }

    // Detect CD-ROM image
    QString lower = path.toLower();
    m_is_cdrom = lower.endsWith(".cdi") || lower.endsWith(".iso");

    // Init core — JaguarInit calls DACInit internally which opens SDL audio
    JaguarInit();
    m_initialized = true;

    // Set framebuffer — pitch must be 1024 (power-of-2) as VJ requires
    JaguarSetScreenBuffer(m_framebuffer.data());
    JaguarSetScreenPitch(JAG_TEX_WIDTH);

    // Load ROM
    QByteArray pathBytes = path.toLocal8Bit();
    bool ok = JaguarLoadFile(pathBytes.data());
    if (!ok) {
        qWarning() << "JaguarSystem: failed to load" << path;
        DACDone();
        JaguarDone();
        m_initialized = false;
        return false;
    }

    // Init VST host.
    // Nota: o core de referência não expõe DACSetPostProcCallback.
    VSTHost::instance().init(48000.0, 1024);

    JaguarReset();

    memset(m_joypad0, 0, sizeof(m_joypad0));
    memset(m_joypad1, 0, sizeof(m_joypad1));
    m_frame.fill(Qt::black);

    qDebug() << "JaguarSystem: loaded" << QFileInfo(path).fileName()
             << (m_is_cdrom ? "(CD-ROM)" : "(cartridge)");
    return true;
}

void JaguarSystem::start()
{
    m_running = true;
    // Unpause DAC audio thread
    if (m_initialized)
        DACPauseAudioThread(false);
}

void JaguarSystem::stop()
{
    m_running = false;
    if (m_initialized)
        DACPauseAudioThread(true);
}

void JaguarSystem::step()
{
    if (!m_running || !m_initialized)
        return;

    // Feed joystick state to VJ
    memcpy(joypad0Buttons, m_joypad0, sizeof(m_joypad0));
    memcpy(joypad1Buttons, m_joypad1, sizeof(m_joypad1));

    // Run one full Jaguar frame (until VBL sets frameDone), with robust guards.
    // A too-small wall-time budget here can stop before TOM/OP finish, causing blue screen.
    extern bool frameDone;
    frameDone = false;

    QElapsedTimer guard;
    guard.start();

    int iterations = 0;
    constexpr int kMaxIterations = 1000000; // hard cap against runaway loops
    constexpr qint64 kMaxMs = 100;          // allow enough time to reach VBL/frameDone

    while (!frameDone && iterations < kMaxIterations && guard.elapsed() < kMaxMs) {
        double timeToNextEvent = GetTimeToNextEvent();

        // Avoid 0/negative/NaN/huge values causing busy-loop or stalls.
        if (!(timeToNextEvent > 0.0) || timeToNextEvent > 2000.0) {
            timeToNextEvent = 1.0;
        }

        m68k_execute(USEC_TO_M68K_CYCLES(timeToNextEvent));

        if (vjs.GPUEnabled) {
            GPUExec(USEC_TO_RISC_CYCLES(timeToNextEvent));
        }
        // DSP timing is handled by Jaguar/JERRY event path in this integration build.

        // Advance Jaguar event queue (halfline/video IRQ/etc), which is what eventually sets frameDone.
        HandleNextEvent();
        ++iterations;
    }

    if (!frameDone) {
        static int warnCounter = 0;
        if ((warnCounter++ & 0x3F) == 0) {
            qWarning() << "JaguarSystem::step: frame did not complete"
                       << "iterations=" << iterations
                       << "elapsedMs=" << guard.elapsed()
                       << "GPU=" << vjs.GPUEnabled
                       << "DSP=" << vjs.DSPEnabled;
        }
    }




    // Override buggy TOMGetVideoModeWidth with manual calc
    uint16_t vmode = GET16(tomRam8, VMODE);
    uint16_t hdb1 = GET16(tomRam8, HDB1);
    uint16_t hde = GET16(tomRam8, HDE);
    uint8_t pwidth = ((vmode & 0x0E00) >> 9) + 1;
    int calc_w = (hde - hdb1) / pwidth;
    int visW = std::max(320, calc_w);
    int visH_raw = TOMGetVideoModeHeight();
    double interlace_mult = (TOMGetVP() & 0x0001) ? 1.0 : 2.0;
    int visH = static_cast<int>(visH_raw * interlace_mult);

    // Clamp to safe bounds
    visW = std::clamp(visW, 256, 400);
    visH = std::clamp(visH, 200, 576);

    qDebug() << QString("TOM DEBUG VMODE=0x%1 pwidth=%2 HDB1=%3 HDE=%4 calc_w=%5 vis=%6x%7 fb0=0x%8")
                .arg(vmode,4,16).arg(pwidth).arg(hdb1).arg(hde).arg(calc_w).arg(visW).arg(visH).arg(m_framebuffer[0],8,16);

    if (m_frame.size() != QSize(visW, visH))
        m_frame = QImage(visW, visH, QImage::Format_ARGB32);

    // Full copy with pitch awareness, safe bounds
    for (int y = 0; y < visH && y < JAG_TEX_HEIGHT; ++y) {
        const uint32_t* src_line = m_framebuffer.data() + y * JAG_TEX_WIDTH;
        QRgb* dst_line = reinterpret_cast<QRgb*>(m_frame.scanLine(y));
        for (int x = 0; x < visW && x < JAG_TEX_WIDTH; ++x) {
            dst_line[x] = qRgba(
                (src_line[x] >> 24) & 0xFF,
                (src_line[x] >> 16) & 0xFF,
                (src_line[x] >> 8) & 0xFF,
                0xFF
            );
        }
    }



}

QImage JaguarSystem::getFrame() const
{
    return m_frame;
}

bool JaguarSystem::saveState(const QString &path)
{
    Q_UNUSED(path);
    return false;
}

bool JaguarSystem::loadState(const QString &path)
{
    Q_UNUSED(path);
    return false;
}

void JaguarSystem::setJoystickState(const JoystickState &s)
{
    // JoystickState is the generic 2600 state — for Jaguar we use
    // the Jaguar-specific state set via setJaguarInputState().
    // Map the generic state to basic Jaguar directions + B button
    // so the emulator is at least navigable without full Jaguar bindings.
    memset(m_joypad0, 0, sizeof(m_joypad0));
    if (s.up)    m_joypad0[BUTTON_U] = 1;
    if (s.down)  m_joypad0[BUTTON_D] = 1;
    if (s.left)  m_joypad0[BUTTON_L] = 1;
    if (s.right) m_joypad0[BUTTON_R] = 1;
    if (s.fire)  m_joypad0[BUTTON_B] = 1;
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
    // Numpad (s = *, d = #)
    if (s.star)   m_joypad0[BUTTON_s]      = 1;
    if (s.hash)   m_joypad0[BUTTON_d]      = 1;
    // 0-9
    if (s.num[0]) m_joypad0[BUTTON_0]      = 1;
    if (s.num[1]) m_joypad0[BUTTON_1]      = 1;
    if (s.num[2]) m_joypad0[BUTTON_2]      = 1;
    if (s.num[3]) m_joypad0[BUTTON_3]      = 1;
    if (s.num[4]) m_joypad0[BUTTON_4]      = 1;
    if (s.num[5]) m_joypad0[BUTTON_5]      = 1;
    if (s.num[6]) m_joypad0[BUTTON_6]      = 1;
    if (s.num[7]) m_joypad0[BUTTON_7]      = 1;
    if (s.num[8]) m_joypad0[BUTTON_8]      = 1;
    if (s.num[9]) m_joypad0[BUTTON_9]      = 1;
}

// Audio — VJ's DAC manages SDL audio internally via DACInit/DACDone.
// These methods are no-ops; we just control the DAC pause state.
void JaguarSystem::initAudio(const QString &)
{
    if (m_initialized && m_running)
        DACPauseAudioThread(false);
}

void JaguarSystem::closeAudio()
{
    if (m_initialized)
        DACPauseAudioThread(true);
}

void JaguarSystem::setAudioVolume(int percent)
{
    m_audio_volume = std::clamp(percent, 0, 100);
}

void JaguarSystem::setAudioEnabled(bool enabled)
{
    m_audio_enabled = enabled;
    if (m_initialized)
        DACPauseAudioThread(!enabled);
}
