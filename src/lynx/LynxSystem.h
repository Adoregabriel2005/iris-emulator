#pragma once

#include "IEmulatorCore.h"
#include "SDLInput.h"

#include <SDL.h>
#include <cstdint>
#include <vector>
#include <QString>
#include <QImage>
#include <QDataStream>

class GearlynxCore;

// Top-level Atari Lynx emulator — implements IEmulatorCore for MainWindow integration.
// Lynx hardware:
//   CPU: WDC 65C02 @ 4 MHz
//   RAM: 64 KB
//   Suzy: sprite engine + math + input ($FC00–$FCFF)
//   Mikey: timers + audio + display + palette ($FD00–$FDFF)
//   Display: 160×102, 4bpp, ~75 Hz
class LynxSystem : public IEmulatorCore
{
    Q_OBJECT
public:
    explicit LynxSystem(QObject *parent = nullptr);
    ~LynxSystem() override;

    // IEmulatorCore interface
    bool loadROM(const QString &path) override;
    void start() override;
    void stop() override;
    void step() override;                // one frame (~53333 CPU cycles @ 75 Hz)
    QImage getFrame() const override;
    bool isRunning() const override { return m_running; }
    bool biosFound() const { return m_bios_found; }

    bool saveState(const QString &path) override;
    bool loadState(const QString &path) override;

    void setJoystickState(const JoystickState &s) override;  // legacy — maps 2600 actions to Lynx
    void setLynxInputState(const LynxInputState &s);         // native Lynx input with correct semantics

    void initAudio(const QString &deviceName = QString()) override;
    void closeAudio() override;
    void setAudioVolume(int percent) override;
    void setAudioEnabled(bool enabled) override;
    bool isAudioEnabled() const override { return m_audio_enabled; }

    // Debug information access (for DebugWindow)
    struct DebugInfo {
        uint16_t pc = 0;
        uint8_t a = 0, x = 0, y = 0, sp = 0, p = 0;
        uint64_t totalCycles = 0;
        bool isRunning = false;
    };
    DebugInfo getDebugInfo() const;

private:
    bool m_running = false;
    bool m_bios_found = false;
    GearlynxCore *m_core = nullptr;
    QImage m_frame;
    std::vector<uint8_t> m_frame_buffer;
    std::vector<int16_t> m_sample_buffer;
    uint16_t m_lynx_keys = 0;

    SDL_AudioDeviceID m_audio_device = 0;
    int m_audio_sample_rate = 44100;
    bool m_audio_enabled = true;
    int m_audio_volume = 80;
};
