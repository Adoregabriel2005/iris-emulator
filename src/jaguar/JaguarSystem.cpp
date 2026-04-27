// VJ core headers MUST come before SDLInput.h
// joystick.h from VJ defines BUTTON_U etc. which must not conflict with SDL types
#include "JaguarSystem.h"

#include "jaguar.h"
#include "tom.h"
#include "jerry.h"
#include "dsp.h"
#include "gpu.h"
#include "memory.h"
#include "settings.h"
#include "joystick.h"
#include "dac.h"
#include "file.h"
#include "event.h"
#include "modelsBIOS.h"
#include "m68000/m68kinterface.h"
#include "log.h"

// SDLInput after VJ core to get JoystickState/JaguarInputState definitions
#include "SDLInput.h"

#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>
#include <QDebug>
#include <cstring>
#include <algorithm>


extern VJSettings  vjs;
extern bool        jaguarCartInserted;
extern uint8_t     jagMemSpace[];
extern bool        frameDone;
extern uint32_t    jaguarMainROMCRC32;
extern uint32_t    jaguarRunAddress;
extern unsigned char* joypad0Buttons;
extern unsigned char* joypad1Buttons;

#include "JaguarSafeWrite.h"

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
    if (m_initialized) {
        DACDone();
        JaguarDone();
        m_initialized = false;
    }
    LogDone();
}

bool JaguarSystem::loadROM(const QString &path)
{
    if (m_initialized) {
        DACDone();
        JaguarDone();
        m_initialized = false;
    }

    QSettings qs;
    memset(&vjs, 0, sizeof(vjs));
    vjs.hardwareTypeNTSC        = (qs.value("Jaguar/TVStandard", "NTSC").toString() != "PAL");
    vjs.GPUEnabled              = true;
    vjs.DSPEnabled              = true;
    vjs.usePipelinedDSP         = false;
    vjs.audioEnabled            = true;
    vjs.useJaguarBIOS           = false;
    vjs.useRetailBIOS           = false;
    vjs.useFastBlitter          = false;
    vjs.DRAM_size               = 0x200000;
    vjs.jaguarModel             = JAG_M_SERIES;
    vjs.biosType                = BT_M_SERIES;
    vjs.allowM68KExceptionCatch = false;
    vjs.allowWritesToROM        = true;

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

    QByteArray pathBytes = path.toLocal8Bit();
    if (!JaguarLoadFile(pathBytes.data())) {
        qWarning() << "JaguarSystem: failed to load" << path;
        DACDone();
        JaguarDone();
        m_initialized = false;
        return false;
    }

    m_activePatch = nullptr;
    for (int i = 0; i < kJagPatchDBCount; ++i) {
        if (kJagPatchDB[i].crc32 != 0 &&
            kJagPatchDB[i].crc32 == jaguarMainROMCRC32) {
            m_activePatch = &kJagPatchDB[i];
            qDebug() << "JaguarSystem: patch active for" << kJagPatchDB[i].name;
            break;
        }
    }

    m_doomInStall      = false;
    m_doomTicStall     = 0;
    m_cfInGame         = false;
    m_cfCurrentFbAddr  = 0;
    m_cmUploadPrgAddr  = 0;
    m_cmCurrentBufAddr = 0;

    JaguarReset();
    DACPauseAudioThread(false);

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

    applyPatches();

    const uint16_t vmode  = GET16(tomRam8, 0x28);
    const uint16_t hdb1   = GET16(tomRam8, 0x38);
    const uint16_t hde    = GET16(tomRam8, 0x3C);
    const int      pwidth = ((vmode & 0x0E00) >> 9) + 1;
    const int      calcW  = (hde > hdb1) ? (hde - hdb1) / pwidth : 0;
    const int visW = std::clamp(calcW > 0 ? calcW : VIRTUAL_SCREEN_WIDTH, 256, 800);
    const int visH = std::clamp(
        vjs.hardwareTypeNTSC ? VIRTUAL_SCREEN_HEIGHT_NTSC : VIRTUAL_SCREEN_HEIGHT_PAL,
        100, 576);

    if (m_frame.size() != QSize(visW, visH))
        m_frame = QImage(visW, visH, QImage::Format_RGB32);

    for (int y = 0; y < visH && y < kTexH; ++y) {
        const uint32_t *src = m_framebuffer.data() + y * kTexW;
        QRgb *dst = reinterpret_cast<QRgb *>(m_frame.scanLine(y));
        for (int x = 0; x < visW && x < kTexW; ++x) {
            const uint32_t p = src[x];
            dst[x] = qRgb((p >> 24) & 0xFF, (p >> 16) & 0xFF, (p >> 8) & 0xFF);
        }
    }
}

QImage JaguarSystem::getFrame() const { return m_frame; }

void JaguarSystem::applyPatches()
{
    if (!m_activePatch) return;
    const uint32_t pc = m68k_get_reg(nullptr, M68K_REG_PC);
    for (int i = 0; i < m_activePatch->count; ++i) {
        if (m_activePatch->entries[i].m68kPC != pc) continue;
        switch (m_activePatch->entries[i].action) {
        case JagPatchAction::Doom_MenuStall:     patch_Doom_MenuStall();     break;
        case JagPatchAction::CF_DrawSky:         patch_CF_DrawSky();         break;
        case JagPatchAction::CF_BufferFlip:      patch_CF_BufferFlip();      break;
        case JagPatchAction::CF_GameBegin:       patch_CF_GameBegin();       break;
        case JagPatchAction::CF_GameEnd:         patch_CF_GameEnd();         break;
        case JagPatchAction::AvP_SetVidParams:   patch_AvP_SetVidParams();   break;
        case JagPatchAction::CM_UploadGPU:       patch_CM_UploadGPU();       break;
        case JagPatchAction::CM_UploadGPUFinish: patch_CM_UploadGPUFinish(); break;
        case JagPatchAction::CM_BgBlit:          patch_CM_BgBlit();          break;
        case JagPatchAction::CM_EndFrame:        patch_CM_EndFrame();        break;
        default: break;
        }
    }
}

void JaguarSystem::patch_Doom_MenuStall()
{
    const uint32_t ticCount = JaguarReadLong(0x00047DA4);
    if (m_doomInStall) {
        if (ticCount >= m_doomTicStall) m_doomInStall = false;
    } else {
        m_doomTicStall = ticCount + 2;
        m_doomInStall  = true;
    }
    if (m_doomInStall)
        m68k_set_reg(M68K_REG_PC, 0x00009CAA - 2);
}

void JaguarSystem::patch_CF_DrawSky()
{
    if (!m_cfInGame) return;
    const uint32_t fbOffset = JaguarReadLong(0x00004044);
    m_cfCurrentFbAddr = 0x000E8000 + fbOffset;
}

void JaguarSystem::patch_CF_BufferFlip()  { m_cfCurrentFbAddr = 0; }
void JaguarSystem::patch_CF_GameBegin()   { m_cfInGame = true; }
void JaguarSystem::patch_CF_GameEnd()     { m_cfInGame = false; }

void JaguarSystem::patch_AvP_SetVidParams()
{
    const uint16_t cur = JaguarReadWord(0x0002EE8C);
    if (cur > 2) JaguarWriteWord(0x0002EE8C, 2);
}

void JaguarSystem::patch_CM_UploadGPU()
{
    m_cmUploadPrgAddr = m68k_get_reg(nullptr, M68K_REG_D0);
}

void JaguarSystem::patch_CM_UploadGPUFinish() {}

void JaguarSystem::patch_CM_BgBlit()
{
    const uint32_t a0     = m68k_get_reg(nullptr, M68K_REG_A0);
    const uint32_t bgAddr = JaguarReadLong(a0);
    if (bgAddr >= 0x00110000 && bgAddr <= 0x00110010) {
        const uint16_t fadeCount = JaguarReadWord(0x00009A50);
        m_cmCurrentBufAddr = (fadeCount == 0) ? bgAddr : 0;
    }
}

void JaguarSystem::patch_CM_EndFrame() { m_cmCurrentBufAddr = 0; }

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
    m_audioVolume = std::clamp(percent, 0, 100);
}

void JaguarSystem::setAudioEnabled(bool enabled)
{
    m_audioEnabled = enabled;
    if (m_initialized) DACPauseAudioThread(!enabled);
}
