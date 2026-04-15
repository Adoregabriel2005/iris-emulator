#include "JaguarSystem.h"
#include "SDLInput.h"
#include "../audio/VSTHost.h"
#include <SDL.h>

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
#include "modelsBIOS.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QDataStream>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <cstring>
#include <algorithm>
#include <cmath>

extern VJSettings vjs;
extern bool startM68KTracing;
extern bool jaguarCartInserted;
extern uint8_t jagMemSpace[];

// -----------------------------------------------------------------------------

JaguarSystem::JaguarSystem(QObject *parent)
    : IEmulatorCore(parent)
    , m_framebuffer(JAG_TEX_WIDTH * JAG_TEX_HEIGHT, 0)
{
    m_frame = QImage(JAG_WIDTH, JAG_HEIGHT, QImage::Format_RGB32);
    m_frame.fill(Qt::black);
    QString logPath = QCoreApplication::applicationDirPath() + "/jaguar_iris.log";
    LogInit(logPath.toLocal8Bit().data());
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

// -----------------------------------------------------------------------------
// loadBIOSFromDisk
// Tenta carregar jagboot.rom do disco. Candidatos em ordem:
//   1. QSettings key "Jaguar/BIOSPath"
//   2. Próximo ao .exe
//   3. Caminho padrão do usuário
// Se encontrar arquivo válido (exatamente 0x20000 bytes), copia para
// jagMemSpace + 0xE00000 e retorna true. Caso contrário retorna false
// e o chamador usa SelectBIOS() com o array interno.
// -----------------------------------------------------------------------------
bool JaguarSystem::loadBIOSFromDisk()
{
    QSettings qs("Underground Software", "Virtual Jaguar");
    QString appDir = QCoreApplication::applicationDirPath();

    QStringList candidates = {
        qs.value("Jaguar/BIOSPath", "").toString(),
        appDir + "/jagboot.rom",
        appDir + "/jaguar.rom",
        appDir + "/jaguar.j64",
        "D:/ROMS/JAGUAR/[BIOS] Atari Jaguar (World)/jagboot.rom",
        "D:/ROMS/JAGUAR/[BIOS] Atari Jaguar (World)/[BIOS] Atari Jaguar (World).j64",
    };

    for (const QString &path : candidates) {
        if (path.isEmpty()) continue;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) continue;
        if (f.size() != 0x20000) continue;
        QByteArray data = f.readAll();
        memcpy(jagMemSpace + 0xE00000, data.constData(), 0x20000);
        qDebug() << "JaguarSystem: BIOS carregada do disco:" << path;
        return true;
    }
    qDebug() << "JaguarSystem: BIOS do disco n\u00e3o encontrada, usando array interno";
    return false;
}

// -----------------------------------------------------------------------------
// loadROM
// Sequence mirrors mainwin.cpp exactly:
//   ReadSettings -> JaguarInit -> SelectBIOS -> JaguarReset -> DACPauseAudioThread(false)
//   -> JaguarLoadFile -> SET32 stack/PC -> m68k_pulse_reset
// -----------------------------------------------------------------------------
bool JaguarSystem::loadROM(const QString &path)
{
    if (m_initialized) {
        VSTHost::instance().shutdown();
        DACDone();
        JaguarDone();
        m_initialized = false;
    }

    // 1. vjs settings
    QSettings qs("Underground Software", "Virtual Jaguar");
    memset(&vjs, 0, sizeof(vjs));
    vjs.hardwareTypeNTSC        = qs.value("hardwareTypeNTSC",   true).toBool();
    vjs.GPUEnabled              = qs.value("GPUEnabled",         true).toBool();
    vjs.DSPEnabled              = qs.value("DSPEnabled",         true).toBool();
    vjs.usePipelinedDSP         = qs.value("usePipelinedDSP",    false).toBool();
    vjs.audioEnabled            = qs.value("audioEnabled",       true).toBool();
    vjs.useJaguarBIOS           = qs.value("useJaguarBIOS",      false).toBool();
    vjs.useRetailBIOS           = qs.value("useRetailBIOS",      false).toBool();
    vjs.useDevBIOS              = qs.value("useDevBIOS",         false).toBool();
    vjs.useFastBlitter          = qs.value("Jaguar/FastBlitter",   false).toBool();
    vjs.biosType                = qs.value("biosType",           BT_M_SERIES).toInt();
    vjs.jaguarModel             = qs.value("jaguarModel",        JAG_M_SERIES).toInt();
    vjs.allowWritesToROM        = true;
    vjs.allowM68KExceptionCatch = false;
    vjs.DRAM_size               = 0x200000;
    vjs.hardwareTypeAlpine      = false;
    vjs.softTypeDebugger        = false;

    {
        QString ep = qs.value("EEPROMs",
            QCoreApplication::applicationDirPath() + "/eeproms/").toString();
        QDir().mkpath(ep);
        strncpy(vjs.EEPROMPath, ep.toLocal8Bit().constData(), MAX_PATH - 1);
    }
    {
        QString rp = qs.value("ROMs",
            QCoreApplication::applicationDirPath() + "/software/").toString();
        strncpy(vjs.ROMPath, rp.toLocal8Bit().constData(), MAX_PATH - 1);
    }

    // 2. SDL audio subsystem
    if (!(SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO))
        SDL_InitSubSystem(SDL_INIT_AUDIO);

    // 3. JaguarInit (calls DACInit internally)
    jaguarCartInserted = true;
    JaguarInit();
    m_initialized = true;

    // 4. BIOS — tenta carregar do disco primeiro, fallback para array interno
    if (vjs.useJaguarBIOS) {
        if (!loadBIOSFromDisk())
            SelectBIOS(vjs.biosType);
    } else {
        SelectBIOS(vjs.biosType);
    }

    // 5. Screen buffer
    JaguarSetScreenBuffer(m_framebuffer.data());
    JaguarSetScreenPitch(JAG_TEX_WIDTH);

    // 6. JaguarReset + start audio (mirrors TogglePowerState(on))
    JaguarReset();
    DACPauseAudioThread(false);

    // 7. Load ROM
    QByteArray pathBytes = path.toLocal8Bit();
    bool ok = JaguarLoadFile(pathBytes.data());
    if (!ok) {
        qWarning() << "JaguarSystem: failed to load" << path;
        DACDone();
        JaguarDone();
        m_initialized = false;
        return false;
    }

    // 8. Boot vectors — BIOS real ou HLE
    // Com BIOS: SetBIOS() copia SP+PC de jagMemSpace+0xE00000 para jaguarMainRAM[0..7]
    //           m68k_pulse_reset() lê esses vetores → CPU inicia pela BIOS
    //           A BIOS roda a intro, inicializa GPU/OP/Blitter e pula pro jogo
    // Sem BIOS: HLE — SP = topo da DRAM, PC = endereço do jogo direto
    if (vjs.useJaguarBIOS) {
        SetBIOS(); // copia [SP, PC] da ROM 0xE00000 → jaguarMainRAM[0..7]
    } else {
        SET32(jaguarMainRAM, 0, vjs.DRAM_size);
        SET32(jaguarMainRAM, 4, jaguarRunAddress);
    }
    m68k_pulse_reset();

    // 9. VST host — no plugin loading, just init gain
    VSTHost::instance().init(48000.0, 2048);
    // Only install SDL wrapper if user has plugins or non-unity gain
    if (VSTHost::instance().pluginCount() > 0 ||
        std::fabs(VSTHost::instance().masterGain() - 1.0f) > 0.01f)
        VSTHost::instance().installSDLCallbackWrapper();

    // 10. Clear input
    memset(m_joypad0, 0, sizeof(m_joypad0));
    memset(m_joypad1, 0, sizeof(m_joypad1));
    m_frame.fill(Qt::black);

    QString lower = path.toLower();
    m_is_cdrom = lower.endsWith(".cdi") || lower.endsWith(".iso");

    qDebug() << "JaguarSystem: loaded" << QFileInfo(path).fileName()
             << (m_is_cdrom ? "(CD-ROM)" : "(cartridge)")
             << "runAddr=" << Qt::hex << jaguarRunAddress
             << "CRC32=" << Qt::hex << jaguarMainROMCRC32;
    return true;
}

// -----------------------------------------------------------------------------

void JaguarSystem::start()
{
    m_running = true;
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

    memcpy(joypad0Buttons, m_joypad0, sizeof(m_joypad0));
    memcpy(joypad1Buttons, m_joypad1, sizeof(m_joypad1));

    JaguarExecuteNew();

    int visW = static_cast<int>(TOMGetVideoModeWidth());
    int visH = vjs.hardwareTypeNTSC ? VIRTUAL_SCREEN_HEIGHT_NTSC
                                    : VIRTUAL_SCREEN_HEIGHT_PAL;
    visW = std::clamp(visW, 160, 800);
    visH = std::clamp(visH, 100, 576);

    if (m_frame.size() != QSize(visW, visH))
        m_frame = QImage(visW, visH, QImage::Format_RGB32);

    for (int y = 0; y < visH; ++y) {
        const uint32_t *src = screenBuffer + y * screenPitch;
        QRgb *dst = reinterpret_cast<QRgb *>(m_frame.scanLine(y));
        for (int x = 0; x < visW; ++x) {
            uint32_t p = src[x];
            dst[x] = qRgb((p >> 24) & 0xFF, (p >> 16) & 0xFF, (p >> 8) & 0xFF);
        }
    }
}

// -----------------------------------------------------------------------------

QImage JaguarSystem::getFrame() const { return m_frame; }
bool JaguarSystem::saveState(const QString &path)
{
    if (!m_initialized) return false;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;

    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::BigEndian);

    // Header
    ds.writeRawData("IRIS_JAG", 8);
    ds << (quint32)1; // version

    // 68K registers
    for (int r = M68K_REG_D0; r <= M68K_REG_A7; r++)
        ds << (quint32)m68k_get_reg(nullptr, (m68k_register_t)r);
    ds << (quint32)m68k_get_reg(nullptr, M68K_REG_PC);
    ds << (quint32)m68k_get_reg(nullptr, M68K_REG_SR);

    // Main RAM (2MB)
    ds.writeRawData(reinterpret_cast<const char*>(jaguarMainRAM), vjs.DRAM_size);

    // TOM RAM (16KB)
    ds.writeRawData(reinterpret_cast<const char*>(TOMGetRamPointer()), 0x4000);

    return f.error() == QFile::NoError;
}

bool JaguarSystem::loadState(const QString &path)
{
    if (!m_initialized) return false;

    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;

    QDataStream ds(&f);
    ds.setByteOrder(QDataStream::BigEndian);

    // Header
    char magic[8];
    ds.readRawData(magic, 8);
    if (memcmp(magic, "IRIS_JAG", 8) != 0) return false;
    quint32 version; ds >> version;
    if (version != 1) return false;

    // 68K registers
    for (int r = M68K_REG_D0; r <= M68K_REG_A7; r++) {
        quint32 v; ds >> v;
        m68k_set_reg((m68k_register_t)r, v);
    }
    quint32 pc, sr; ds >> pc >> sr;
    m68k_set_reg(M68K_REG_PC, pc);
    m68k_set_reg(M68K_REG_SR, sr);

    // Main RAM
    ds.readRawData(reinterpret_cast<char*>(jaguarMainRAM), vjs.DRAM_size);

    // TOM RAM
    ds.readRawData(reinterpret_cast<char*>(TOMGetRamPointer()), 0x4000);

    return f.error() == QFile::NoError;
}

void JaguarSystem::setJoystickState(const JoystickState &s)
{
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
    if (s.star)   m_joypad0[BUTTON_s]      = 1;
    if (s.hash)   m_joypad0[BUTTON_d]      = 1;
    if (s.n1) m_joypad0[BUTTON_1] = 1; if (s.n2) m_joypad0[BUTTON_2] = 1;
    if (s.n3) m_joypad0[BUTTON_3] = 1; if (s.n4) m_joypad0[BUTTON_4] = 1;
    if (s.n5) m_joypad0[BUTTON_5] = 1; if (s.n6) m_joypad0[BUTTON_6] = 1;
    if (s.n7) m_joypad0[BUTTON_7] = 1; if (s.n8) m_joypad0[BUTTON_8] = 1;
    if (s.n9) m_joypad0[BUTTON_9] = 1; if (s.n0) m_joypad0[BUTTON_0] = 1;
}

void JaguarSystem::initAudio(const QString &)
{
    if (m_initialized && m_running) DACPauseAudioThread(false);
}

void JaguarSystem::closeAudio()
{
    if (m_initialized) DACPauseAudioThread(true);
}

void JaguarSystem::setAudioVolume(int percent)
{
    m_audio_volume = std::clamp(percent, 0, 100);
}

void JaguarSystem::setAudioEnabled(bool enabled)
{
    m_audio_enabled = enabled;
    if (m_initialized) DACPauseAudioThread(!enabled);
}
