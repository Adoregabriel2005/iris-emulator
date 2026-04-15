#pragma once

#include <QString>
#include <QObject>
#include <QTimer>
#include <cstdint>

// Discord Rich Presence integration
// Application ID from Discord Developer Portal
// Using a generic Atari emulator app ID — replace with your own
#define IRIS_DISCORD_APP_ID "1491974835323011072"

class DiscordRPC : public QObject
{
    Q_OBJECT
public:
    enum class State { Menu, Playing, Paused };

    explicit DiscordRPC(QObject* parent = nullptr);
    ~DiscordRPC();

    void init();
    void shutdown();

    void setEnabled(bool enabled);
    bool isEnabled() const { return m_enabled; }

    void updatePlaying(const QString& gameTitle, const QString& console, const QString& coverKey);
    void updatePaused(const QString& gameTitle, const QString& console);
    void updateMenu();

private:
    void applyPresence();

    bool        m_enabled     = false;
    bool        m_initialized = false;
    State       m_state       = State::Menu;
    QString     m_game_title;
    QString     m_console;
    QString     m_cover_key;
    int64_t     m_start_time  = 0;
    int         m_reconnect_counter = 0;
    QTimer*     m_pump_timer  = nullptr;
};
