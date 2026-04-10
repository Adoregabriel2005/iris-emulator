#include "DiscordRPC.h"
#include <discord_rpc.h>
#include <QDateTime>
#include <QSettings>
#include <QDebug>
#include <cstring>

DiscordRPC::DiscordRPC(QObject* parent)
    : QObject(parent)
{
    m_pump_timer = new QTimer(this);
    m_pump_timer->setInterval(500);
    connect(m_pump_timer, &QTimer::timeout, this, []() {
        Discord_RunCallbacks();
    });
}

DiscordRPC::~DiscordRPC()
{
    shutdown();
}

void DiscordRPC::init()
{
    QSettings s;
    m_enabled = s.value("Discord/Enabled", true).toBool();
    if (!m_enabled)
        return;

    DiscordEventHandlers handlers{};
    handlers.ready = [](const DiscordUser* user) {
        qDebug() << "Discord RPC ready! User:" << (user ? user->username : "unknown");
    };
    handlers.errored = [](int errorCode, const char* message) {
        qWarning() << "Discord RPC error" << errorCode << ":" << message;
    };
    handlers.disconnected = [](int errorCode, const char* message) {
        qWarning() << "Discord RPC disconnected" << errorCode << ":" << message;
    };

    Discord_Initialize(IRIS_DISCORD_APP_ID, &handlers, 1, nullptr);
    m_initialized = true;
    m_pump_timer->start();
    updateMenu();
    qDebug() << "Discord RPC initialized with App ID:" << IRIS_DISCORD_APP_ID;
}

void DiscordRPC::shutdown()
{
    if (m_initialized) {
        m_pump_timer->stop();
        Discord_ClearPresence();
        Discord_Shutdown();
        m_initialized = false;
    }
}

void DiscordRPC::setEnabled(bool enabled)
{
    m_enabled = enabled;
    QSettings s;
    s.setValue("Discord/Enabled", enabled);

    if (enabled && !m_initialized)
        init();
    else if (!enabled && m_initialized)
        shutdown();
}

void DiscordRPC::updatePlaying(const QString& gameTitle, const QString& console, const QString& coverKey)
{
    m_state      = State::Playing;
    m_game_title = gameTitle;
    m_console    = console;
    m_cover_key  = coverKey;
    m_start_time = QDateTime::currentSecsSinceEpoch();
    applyPresence();
}

void DiscordRPC::updatePaused(const QString& gameTitle, const QString& console)
{
    m_state      = State::Paused;
    m_game_title = gameTitle;
    m_console    = console;
    applyPresence();
}

void DiscordRPC::updateMenu()
{
    m_state      = State::Menu;
    m_game_title.clear();
    m_console.clear();
    applyPresence();
}

void DiscordRPC::applyPresence()
{
    if (!m_initialized || !m_enabled)
        return;

    DiscordRichPresence p{};

    QByteArray detailsUtf8, stateUtf8, largeTextUtf8, smallTextUtf8;

    switch (m_state) {
    case State::Menu:
        detailsUtf8  = "No game loaded";
        stateUtf8    = "In menu";
        p.largeImageKey  = "iris_logo";
        p.largeImageText = "Íris Emulator";
        break;

    case State::Playing:
        detailsUtf8  = m_game_title.toUtf8();
        stateUtf8    = m_console.toUtf8();
        largeTextUtf8 = m_game_title.toUtf8();
        smallTextUtf8 = m_console.toUtf8();
        p.largeImageKey  = "iris_logo";
        p.largeImageText = largeTextUtf8.constData();
        p.smallImageKey  = m_console.contains("Lynx", Qt::CaseInsensitive) ? "lynx_icon" : "atari2600_icon";
        p.smallImageText = smallTextUtf8.constData();
        p.startTimestamp = m_start_time;
        break;

    case State::Paused:
        detailsUtf8  = m_game_title.toUtf8();
        stateUtf8    = "Paused";
        largeTextUtf8 = m_game_title.toUtf8();
        p.largeImageKey  = "iris_logo";
        p.largeImageText = largeTextUtf8.constData();
        p.smallImageKey  = "pause_icon";
        p.smallImageText = "Paused";
        break;
    }

    p.details = detailsUtf8.constData();
    p.state   = stateUtf8.constData();

    Discord_UpdatePresence(&p);
}
