#pragma once

#include "IEmulatorCore.h"

#include "JaguarPatchDB.h"
#include "tom_utils.h"

#include <QImage>
#include <QString>
#include <cstdint>
#include <vector>

// Forward declarations — full types included in JaguarSystem.cpp
struct JoystickState;
struct JaguarInputState;

class JaguarSystem : public IEmulatorCore
{
    Q_OBJECT
public:
    explicit JaguarSystem(QObject *parent = nullptr);
    ~JaguarSystem() override;

    bool   loadROM(const QString &path) override;
    void   start()   override;
    void   stop()    override;
    void   step()    override;
    QImage getFrame() const override;
    bool   isRunning() const override { return m_running; }

    bool saveState(const QString &path) override;
    bool loadState(const QString &path) override;

    void setJoystickState(const JoystickState &s) override;
    void setJaguarInputState(const JaguarInputState &s);

    void initAudio(const QString &deviceName = QString()) override;
    void closeAudio() override;
    void setAudioVolume(int percent) override;
    void setAudioEnabled(bool enabled) override;
    bool isAudioEnabled() const override { return m_audioEnabled; }

private:
    void applyPatches();

    void patch_Doom_MenuStall();
    void patch_CF_DrawSky();
    void patch_CF_BufferFlip();
    void patch_CF_GameBegin();
    void patch_CF_GameEnd();
    void patch_AvP_SetVidParams();
    void patch_CM_UploadGPU();
    void patch_CM_UploadGPUFinish();
    void patch_CM_BgBlit();
    void patch_CM_EndFrame();

    static constexpr int kTexW = 1024;
    static constexpr int kTexH = 512;
    std::vector<uint32_t> m_framebuffer;
    QImage m_frame;

    bool m_running      = false;
    bool m_initialized  = false;
    bool m_audioEnabled = true;
    int  m_audioVolume  = 80;

    const JagGamePatch* m_activePatch = nullptr;

    bool     m_doomInStall  = false;
    uint32_t m_doomTicStall = 0;

    bool     m_cfInGame        = false;
    uint32_t m_cfCurrentFbAddr = 0;

    uint32_t m_cmUploadPrgAddr  = 0;
    uint32_t m_cmCurrentBufAddr = 0;

    uint8_t m_joypad0[21] = {};
    uint8_t m_joypad1[21] = {};
};
