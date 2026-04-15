#pragma once

#include <QObject>
#include <QImage>
#include <QByteArray>
#include <QString>
#include <QDataStream>
#include <SDL.h>
#include "IEmulatorCore.h"
#include "CPU6507.h"
#include "TIA.h"
#include "RIOT.h"
#include "SDLInput.h"
#include "Mapper.h"
#include "MapperFactory.h"
#include <memory>

class EmulatorCore : public IEmulatorCore
{
    Q_OBJECT
public:
    explicit EmulatorCore(QObject *parent = nullptr);
    ~EmulatorCore() override;

    bool loadROM(const QString &path) override;
    void start() override;
    void stop() override;
    void step() override;
    QImage getFrame() const override;
    bool saveState(const QString &path) override;
    bool loadState(const QString &path) override;
    QByteArray dumpState() const;
    bool loadStateFromData(const QByteArray &data);
    bool isRunning() const override { return m_running; }

    // TV standard (palette + timing) — 2600-specific
    void setTVStandard(TVStandard std);
    TVStandard getTVStandard() const;

    // feed input state from SDLInput (from UI thread)
    void setJoystickState(const JoystickState &s) override;

    // Audio control
    void initAudio(const QString& deviceName = QString()) override;
    void closeAudio() override;
    void setAudioVolume(int percent) override; // 0-100
    void setAudioEnabled(bool enabled) override;
    bool isAudioEnabled() const override { return m_audio_enabled; }

private:
    // SDL audio callback
    static void audioCallback(void* userdata, Uint8* stream, int len);

    // memory bus handlers used by CPU
    uint8_t busRead(uint16_t addr) const;
    void busWrite(uint16_t addr, uint8_t val);

    QByteArray romData;
    TIA *tia = nullptr;
    RIOT *riot = nullptr;
    CPU6507 *cpu = nullptr;
    std::unique_ptr<Mapper> mapper;
    uint8_t ram128[128]; // internal RAM mapped at 0x0080-0x00FF

    QImage frameBuffer;
    bool m_running = false;
    JoystickState m_prev_js;
    QString romPath;
    int cyclesPerFrame = 19886; // approx NTSC 1.193191 MHz / 60
    int m_tiaClocksAdvanced = 0; // TIA clocks advanced during current instruction via cycle callback

    // Audio
    SDL_AudioDeviceID m_audio_device = 0;
    int m_audio_sample_rate = 44100;
    bool m_audio_enabled = true;
    int m_audio_volume = 80; // 0-100
};
