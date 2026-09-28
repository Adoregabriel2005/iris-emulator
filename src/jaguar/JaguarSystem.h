#pragma once

#include "IEmulatorCore.h"

#include <SDL.h>

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
    // TOM can output 1304 pixels at PWIDTH=1 and 512 PAL interlaced lines.
    static constexpr int kTexW = 2048;
    static constexpr int kTexH = 512;
    std::vector<uint32_t> m_framebuffer;
    QImage m_frame;
    SDL_AudioDeviceID m_audioDevice = 0;
    bool m_running = false;
    bool m_initialized = false;
    bool m_audioEnabled = true;
    int m_audioVolume = 80;

    uint8_t m_joypad0[21] = {};
    uint8_t m_joypad1[21] = {};
};
