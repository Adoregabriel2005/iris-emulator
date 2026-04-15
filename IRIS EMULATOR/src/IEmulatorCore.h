#pragma once

#include <QObject>
#include <QImage>
#include <QByteArray>
#include <QString>

struct JoystickState;

// Abstract interface for emulator cores (Atari 2600, Atari Lynx, etc.)
// MainWindow interacts exclusively through this interface.
class IEmulatorCore : public QObject
{
    Q_OBJECT
public:
    explicit IEmulatorCore(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~IEmulatorCore() = default;

    virtual bool loadROM(const QString &path) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void step() = 0;
    virtual QImage getFrame() const = 0;
    virtual bool isRunning() const = 0;

    virtual bool saveState(const QString &path) = 0;
    virtual bool loadState(const QString &path) = 0;

    virtual void setJoystickState(const JoystickState &s) = 0;

    // Audio
    virtual void initAudio(const QString& deviceName = QString()) = 0;
    virtual void closeAudio() = 0;
    virtual void setAudioVolume(int percent) = 0;
    virtual void setAudioEnabled(bool enabled) = 0;
    virtual bool isAudioEnabled() const = 0;
};
