#pragma once

#include "IEmulatorCore.h"
#include "SDLInput.h"
#include <QImage>
#include <QString>
#include <cstdint>
#include <vector>

// Atari Jaguar emulator — wraps Virtual Jaguar RX core
// CPU: Motorola 68000 @ 13.3 MHz
// Tom: GPU + Blitter + Object Processor @ 26.6 MHz
// Jerry: DSP + DAC + Joystick + Timers @ 26.6 MHz
// RAM: 2MB DRAM
class JaguarSystem : public IEmulatorCore
{
    Q_OBJECT
public:
    explicit JaguarSystem(QObject *parent = nullptr);
    ~JaguarSystem() override;

    bool loadROM(const QString &path) override;
    void start() override;
    void stop() override;
    void step() override;
    QImage getFrame() const override;
    bool isRunning() const override { return m_running; }

    bool saveState(const QString &path) override;
    bool loadState(const QString &path) override;

    void setJoystickState(const JoystickState &s) override;
    void setJaguarInputState(const JaguarInputState &s);

    // Audio — managed internally by VJ's DAC/SDL subsystem
    void initAudio(const QString &deviceName = QString()) override;
    void closeAudio() override;
    void setAudioVolume(int percent) override;
    void setAudioEnabled(bool enabled) override;
    bool isAudioEnabled() const override { return m_audio_enabled; }

    bool isCDROM() const { return m_is_cdrom; }

private:
    bool m_running     = false;
    bool m_initialized = false;
    bool m_is_cdrom    = false;

    static constexpr int JAG_TEX_WIDTH  = 1024;  // power-of-2 pitch required by VJ
    static constexpr int JAG_TEX_HEIGHT = 512;
    static constexpr int JAG_WIDTH      = 326;   // default visible width
    static constexpr int JAG_HEIGHT     = 240;   // default visible height (NTSC)
    std::vector<uint32_t> m_framebuffer;
    QImage m_frame;

    // VJ globals (tom.h/jaguar.h)
    // No local state needed - use extern tomWidth/tomHeight/screenBuffer

    bool m_audio_enabled = true;
    int  m_audio_volume  = 80;

    uint8_t m_joypad0[21] = {};
    uint8_t m_joypad1[21] = {};
};
