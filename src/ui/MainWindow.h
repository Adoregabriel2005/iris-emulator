#pragma once

#include <QtCore/QElapsedTimer>
#include <QtCore/QFileInfo>
#include <QtCore/QStandardPaths>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <functional>
#include <optional>

#include "ui_MainWindow.h"

class QProgressBar;
class QActionGroup;

class GameListWidget;
class DisplayWidget;
class OverlayWidget;
class EmulatorCore;
class IEmulatorCore;
class LynxSystem;
class JaguarSystem;
class SDLInput;
class DiscordRPC;
class DebugWindow;

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    static const char* ROM_FILE_FILTER;

    MainWindow();
    ~MainWindow();

    void initialize();
    void loadROMFromCommandLine(const QString& path);

    QLabel* getStatusFPSWidget() const { return m_status_fps_widget; }
    QLabel* getStatusResolutionWidget() const { return m_status_resolution_widget; }

public Q_SLOTS:
    void refreshGameList(bool invalidate_cache);
    void onStatusMessage(const QString& message);

private Q_SLOTS:
    void onStartFileActionTriggered();
    void onViewToolbarActionToggled(bool checked);
    void onViewLockToolbarActionToggled(bool checked);
    void onViewStatusBarActionToggled(bool checked);
    void onViewGameListActionTriggered();
    void onViewGameGridActionTriggered();
    void onViewSystemDisplayTriggered();
    void onAboutActionTriggered();

    void onGameListEntryActivated();
    void onGameListSelectionChanged();

    void onPowerOffActionTriggered();
    void onResetActionTriggered();
    void onPauseActionToggled(bool checked);
    void onScreenshotActionTriggered();
    void onFullscreenActionTriggered();

    void onLoadStateMenuAboutToShow();
    void onSaveStateMenuAboutToShow();

    void onTVStandardChanged();

    void onSettingsTriggeredFromToolbar();
    void onToggleConsoleModeTriggered();
    void onAddGameDirectoryTriggered();

    void onEmulationStarted();
    void onEmulationStopped();
    void onFrameReady(const QImage& frame);

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    void setupAdditionalUi();
    void connectSignals();

    void saveStateToConfig();
    void restoreStateFromConfig();

    void updateEmulationActions(bool running);
    void updateGameGridActions(bool show_game_grid);
    void updateWindowTitle();
    QString getConsoleModeName() const;

    bool isShowingGameList() const;
    void switchToGameListView();
    void switchToEmulationView();

    QWidget* getContentParent();
    void setProgressBar(int current, int total);
    void clearProgressBar();

    enum class ConsoleMode { Auto, Atari2600, AtariLynx, AtariJaguar };

    void setConsoleMode(ConsoleMode mode);
    void updateConsoleModeActionText();

    void startROM(const QString& path);
    void stopEmulation();
    void applyVideoSettings();

    void loadSaveStateSlot(int slot);
    void saveSaveStateSlot(int slot);
    void populateLoadStateMenu(QMenu* menu);
    void populateSaveStateMenu(QMenu* menu);

    Ui::MainWindow m_ui;

    GameListWidget* m_game_list_widget = nullptr;
    DisplayWidget* m_display_widget = nullptr;
    OverlayWidget* m_overlay_widget = nullptr;
    EmulatorCore* m_emu_core = nullptr;
    LynxSystem*    m_lynx_core    = nullptr;
    JaguarSystem*  m_jaguar_core  = nullptr;
    IEmulatorCore* m_active_core = nullptr; // points to whichever core is running
    SDLInput* m_sdl_input = nullptr;

    QProgressBar* m_status_progress_widget = nullptr;
    QLabel* m_status_verbose_widget = nullptr;
    QLabel* m_status_fps_widget = nullptr;
    QLabel* m_status_resolution_widget = nullptr;

    QActionGroup* m_tv_standard_group = nullptr;

    QTimer* m_emulation_timer = nullptr;
    QAction* m_toggle_console_mode_action = nullptr;
    ConsoleMode m_console_mode = ConsoleMode::Auto;
    DiscordRPC*  m_discord      = nullptr;
    DebugWindow* m_debug_window = nullptr;

    bool m_is_running = false;
    bool m_was_paused = false;
    int m_fps_frame_count = 0;
    QElapsedTimer m_fps_timer;
    QString m_current_rom_path;
    QString m_current_game_title;
};

extern MainWindow* g_main_window;
