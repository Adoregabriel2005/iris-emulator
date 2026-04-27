#include "MainWindow.h"
#include "DisplayWidget.h"
#include "GameList/GameListWidget.h"
#include "OverlayWidget.h"
#include "Themes.h"
#include "EmulatorCore.h"
#include "LynxSystem.h"
#include "JaguarSystem.h"
#include "SDLInput.h"
#include "ControllerSettingsDialog.h"
#include "SettingsDialog.h"
#include "CoverDownloadDialog.h"
#include "DebugWindow.h"
#include "JaguarDebugWindow.h"
#include "DiscordRPC.h"

#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QSettings>
#include <QtCore/QTimer>
#include <QtGui/QActionGroup>
#include <QtGui/QCloseEvent>
#include <QtGui/QDragEnterEvent>
#include <QtGui/QDropEvent>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>
#include <QtWidgets/QToolButton>
#include <QtCore/QMimeData>

const char* MainWindow::ROM_FILE_FILTER =
    QT_TRANSLATE_NOOP("MainWindow",
        "All Supported ROMs (*.bin *.a26 *.rom *.lnx *.lyx *.j64 *.jag *.zip);;"
        "Atari 2600 ROMs (*.bin *.a26 *.rom);;"
        "Atari Lynx ROMs (*.lnx *.lyx);;"
        "Atari Jaguar ROMs (*.j64 *.jag);;"
        "All Files (*.*)");

MainWindow* g_main_window = nullptr;

MainWindow::MainWindow()
{
    g_main_window = this;
}

MainWindow::~MainWindow()
{
    stopEmulation();
    delete m_sdl_input;
    m_sdl_input = nullptr;
    if (g_main_window == this)
        g_main_window = nullptr;
}

void MainWindow::initialize()
{
    m_ui.setupUi(this);
    setupAdditionalUi();
    connectSignals();
    restoreStateFromConfig();

    // Wizard de primeiro uso: pedir diretório de ROMs se não tiver nenhum
    {
        QSettings s;
        QStringList dirs = s.value("GameList/Directories").toStringList();
        if (dirs.isEmpty()) {
            QMessageBox msg(this);
            msg.setWindowTitle(tr("Welcome to Íris Emulator!"));
            msg.setIconPixmap(QPixmap(":/iris_icon.svg").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            msg.setText(tr(
                "<b>Welcome to Íris Emulator!</b><br><br>"
                "To get started, add a folder containing your ROM files.<br>"
                "Supported formats: <code>.bin .a26 .rom</code> (Atari 2600) · <code>.lnx .lyx</code> (Atari Lynx)<br><br>"
                "<b>Note:</b> ROMs are not included and cannot be provided.<br>"
                "You must own the original cartridges to legally use ROM files."));
            msg.setStandardButtons(QMessageBox::Ok | QMessageBox::Open);
            msg.button(QMessageBox::Open)->setText(tr("Add ROM Folder"));
            if (msg.exec() == QMessageBox::Open) {
                QString dir = QFileDialog::getExistingDirectory(this, tr("Select ROM Folder"));
                if (!dir.isEmpty()) {
                    dirs.append(dir);
                    s.setValue("GameList/Directories", dirs);
                }
            }
        }
    }

    switchToGameListView();
    updateWindowTitle();
    updateEmulationActions(false);
}

void MainWindow::loadROMFromCommandLine(const QString& path)
{
    startROM(path);
}

QWidget* MainWindow::getContentParent()
{
    return static_cast<QWidget*>(m_ui.mainContainer);
}

void MainWindow::setupAdditionalUi()
{
    // Toolbar visibility
    QSettings settings;
    const bool toolbar_visible = settings.value("UI/ShowToolbar", true).toBool();
    m_ui.actionViewToolbar->setChecked(toolbar_visible);
    m_ui.toolBar->setVisible(toolbar_visible);

    const bool toolbars_locked = settings.value("UI/LockToolbar", false).toBool();
    m_ui.actionViewLockToolbar->setChecked(toolbars_locked);
    m_ui.toolBar->setMovable(!toolbars_locked);
    m_ui.toolBar->setContextMenuPolicy(Qt::PreventContextMenu);

    const bool status_bar_visible = settings.value("UI/ShowStatusBar", true).toBool();
    m_ui.actionViewStatusBar->setChecked(status_bar_visible);
    m_ui.statusBar->setVisible(status_bar_visible);

    const bool show_game_grid = settings.value("UI/GameListGridView", false).toBool();
    updateGameGridActions(show_game_grid);

    // Game list widget
    m_game_list_widget = new GameListWidget(m_ui.mainContainer);
    m_game_list_widget->initialize();
    m_ui.actionGridViewShowTitles->setChecked(m_game_list_widget->getShowGridCoverTitles());
    m_ui.mainContainer->addWidget(m_game_list_widget);

    // Display widget
    m_display_widget = new DisplayWidget(m_ui.mainContainer);
    m_ui.mainContainer->addWidget(m_display_widget);

    // Overlay widget (shown on top of display when paused via Escape)
    m_overlay_widget = new OverlayWidget(m_display_widget);
    m_overlay_widget->hide();

    // Status bar widgets (like PCSX2)
    m_status_progress_widget = new QProgressBar(m_ui.statusBar);
    m_status_progress_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_status_progress_widget->setFixedSize(140, 16);
    m_status_progress_widget->setMinimum(0);
    m_status_progress_widget->setMaximum(100);
    m_status_progress_widget->hide();

    m_status_verbose_widget = new QLabel(m_ui.statusBar);
    m_status_verbose_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_status_verbose_widget->setFixedHeight(16);

    m_status_resolution_widget = new QLabel(m_ui.statusBar);
    m_status_resolution_widget->setFixedHeight(16);
    m_status_resolution_widget->setFixedSize(100, 16);
    m_status_resolution_widget->hide();

    m_status_fps_widget = new QLabel(m_ui.statusBar);
    m_status_fps_widget->setFixedHeight(16);
    m_status_fps_widget->setMinimumWidth(80);
    m_status_fps_widget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_status_fps_widget->hide();

    m_ui.statusBar->addWidget(m_status_verbose_widget, 1);
    m_ui.statusBar->addPermanentWidget(m_status_progress_widget);
    m_ui.statusBar->addPermanentWidget(m_status_resolution_widget);
    m_ui.statusBar->addPermanentWidget(m_status_fps_widget);

    // TV Standard action group (exclusive)
    m_tv_standard_group = new QActionGroup(this);
    m_tv_standard_group->addAction(m_ui.actionTVNTSC);
    m_tv_standard_group->addAction(m_ui.actionTVPAL);
    m_tv_standard_group->addAction(m_ui.actionTVSECAM);

    // Window size sub-menu
    for (int scale = 1; scale <= 6; scale++)
    {
        QAction* action = m_ui.menuWindowSize->addAction(tr("%1x Scale").arg(scale));
        connect(action, &QAction::triggered, [this, scale]() {
            if (m_display_widget)
            {
                // 160 width x ~200 visible height for Atari 2600
                resize(160 * scale + width() - m_ui.mainContainer->width(),
                       200 * scale + height() - m_ui.mainContainer->height());
            }
        });
    }

    // Emulation timer (drives frames when running)
    m_emulation_timer = new QTimer(this);
    m_emulation_timer->setTimerType(Qt::PreciseTimer);

    // Emulator core
    m_emu_core = new EmulatorCore(this);

    // Discord RPC
    m_discord = new DiscordRPC(this);
    m_discord->init();

    // Debug window (created lazily on first open)
    m_debug_window = nullptr;

    // SDL Input
    m_sdl_input = new SDLInput();

    // Let DisplayWidget forward keyboard events to SDLInput
    m_display_widget->setInput(m_sdl_input);

    // Always hide menu bar — single gear button replaces it
    m_ui.menuBar->setVisible(false);

    m_toggle_console_mode_action = new QAction(QIcon::fromTheme(QStringLiteral("swap-left-right-line")), tr("Console Mode: Auto"), this);
    m_toggle_console_mode_action->setStatusTip(tr("Force Atari 2600 or Atari Lynx emulation mode."));
    updateConsoleModeActionText();

    // Create gear (settings) button as first item in toolbar
    auto* gearBtn = new QToolButton(m_ui.toolBar);
    gearBtn->setIcon(QIcon::fromTheme(QStringLiteral("settings-3-line")));
    gearBtn->setText(tr("Settings"));
    gearBtn->setPopupMode(QToolButton::InstantPopup);
    gearBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);

    // Build consolidated menu under gear
    auto* gearMenu = new QMenu(gearBtn);
    gearMenu->addAction(m_ui.actionStartFile);
    gearMenu->addSeparator();
    gearMenu->addAction(m_ui.actionPowerOff);
    gearMenu->addAction(m_ui.actionReset);
    gearMenu->addAction(m_ui.actionPause);
    gearMenu->addSeparator();
    gearMenu->addMenu(m_ui.menuLoadState);
    gearMenu->addMenu(m_ui.menuSaveState);
    gearMenu->addAction(m_ui.actionScreenshot);
    gearMenu->addSeparator();
    gearMenu->addMenu(m_ui.menuTVStandard);
    gearMenu->addSeparator();
    gearMenu->addAction(m_ui.actionViewGameList);
    gearMenu->addAction(m_ui.actionViewGameGrid);
    gearMenu->addAction(m_ui.actionFullscreen);
    gearMenu->addMenu(m_ui.menuWindowSize);
    gearMenu->addSeparator();
    gearMenu->addAction(m_ui.actionAddGameDirectory);
    gearMenu->addAction(m_ui.actionScanForNewGames);
    gearMenu->addAction(m_ui.actionRescanAllGames);
    gearMenu->addSeparator();
    gearMenu->addAction(m_ui.actionAbout);
    gearMenu->addAction(m_ui.actionExit);
    gearMenu->addSeparator();
    gearMenu->addAction(m_toggle_console_mode_action);
    gearMenu->addSeparator();
    // Debug window
    auto* debugAction = new QAction(QIcon::fromTheme("bug-line"), tr("Debug Window"), this);
    debugAction->setShortcut(QKeySequence(Qt::Key_F12));
    connect(debugAction, &QAction::triggered, this, [this]() {
        if (m_jaguar_core && m_active_core == m_jaguar_core) {
            if (!m_jaguar_debug)
                m_jaguar_debug = new JaguarDebugWindow(this);
            m_jaguar_debug->setCore(m_jaguar_core);
            m_jaguar_debug->show();
            m_jaguar_debug->raise();
        } else {
            if (!m_debug_window)
                m_debug_window = new DebugWindow(this);
            m_debug_window->setCore(m_active_core, m_active_core == m_lynx_core);
            m_debug_window->show();
            m_debug_window->raise();
        }
    });
    gearMenu->addAction(debugAction);
    // Cover downloader
    auto* coverDlAction = new QAction(QIcon::fromTheme("image-line"), tr("Download Covers..."), this);
    connect(coverDlAction, &QAction::triggered, this, [this]() {
        QSettings s;
        QString coversDir = s.value("Covers/Directory").toString();
        QStringList roms;
        for (const QString& dir : s.value("GameList/Directories").toStringList()) {
            QDir d(dir);
            for (const QString& f : d.entryList({"*.bin","*.a26","*.rom","*.lnx","*.lyx"}, QDir::Files))
                roms << d.filePath(f);
        }
        CoverDownloadDialog dlg(roms, coversDir, this);
        dlg.exec();
    });
    gearMenu->addAction(coverDlAction);

    gearBtn->setMenu(gearMenu);

    // Insert at the beginning of the toolbar
    QAction* firstAction = m_ui.toolBar->actions().isEmpty() ? nullptr : m_ui.toolBar->actions().first();
    m_ui.toolBar->insertWidget(firstAction, gearBtn);
    m_ui.toolBar->insertSeparator(firstAction);
    if (firstAction)
        m_ui.toolBar->insertAction(firstAction, m_toggle_console_mode_action);
}

void MainWindow::connectSignals()
{
    // System menu
    connect(m_ui.actionStartFile, &QAction::triggered, this, &MainWindow::onStartFileActionTriggered);
    connect(m_ui.actionToolbarStartFile, &QAction::triggered, this, &MainWindow::onStartFileActionTriggered);
    if (m_toggle_console_mode_action)
        connect(m_toggle_console_mode_action, &QAction::triggered, this, &MainWindow::onToggleConsoleModeTriggered);
    connect(m_ui.actionPowerOff, &QAction::triggered, this, &MainWindow::onPowerOffActionTriggered);
    connect(m_ui.actionToolbarPowerOff, &QAction::triggered, this, &MainWindow::onPowerOffActionTriggered);
    connect(m_ui.actionReset, &QAction::triggered, this, &MainWindow::onResetActionTriggered);
    connect(m_ui.actionToolbarReset, &QAction::triggered, this, &MainWindow::onResetActionTriggered);
    connect(m_ui.actionPause, &QAction::toggled, this, &MainWindow::onPauseActionToggled);
    connect(m_ui.actionToolbarPause, &QAction::toggled, this, &MainWindow::onPauseActionToggled);
    connect(m_ui.actionScreenshot, &QAction::triggered, this, &MainWindow::onScreenshotActionTriggered);
    connect(m_ui.actionToolbarScreenshot, &QAction::triggered, this, &MainWindow::onScreenshotActionTriggered);
    connect(m_ui.menuLoadState, &QMenu::aboutToShow, this, &MainWindow::onLoadStateMenuAboutToShow);
    connect(m_ui.menuSaveState, &QMenu::aboutToShow, this, &MainWindow::onSaveStateMenuAboutToShow);
    connect(m_ui.actionToolbarLoadState, &QAction::triggered, this, [this]() { loadSaveStateSlot(1); });
    connect(m_ui.actionToolbarSaveState, &QAction::triggered, this, [this]() { saveSaveStateSlot(1); });
    connect(m_ui.actionExit, &QAction::triggered, this, &MainWindow::close);

    // Settings menu
    connect(m_ui.actionEmulationSettings, &QAction::triggered, this, &MainWindow::onSettingsTriggeredFromToolbar);
    connect(m_ui.actionAudioSettings, &QAction::triggered, this, &MainWindow::onSettingsTriggeredFromToolbar);
    connect(m_ui.actionControllerSettings, &QAction::triggered, this, &MainWindow::onSettingsTriggeredFromToolbar);
    connect(m_ui.actionToolbarSettings, &QAction::triggered, this, &MainWindow::onSettingsTriggeredFromToolbar);
    connect(m_ui.actionToolbarControllerSettings, &QAction::triggered, this, &MainWindow::onSettingsTriggeredFromToolbar);
    connect(m_ui.actionAddGameDirectory, &QAction::triggered, this, &MainWindow::onAddGameDirectoryTriggered);
    connect(m_ui.actionScanForNewGames, &QAction::triggered, [this]() { refreshGameList(false); });
    connect(m_ui.actionRescanAllGames, &QAction::triggered, [this]() { refreshGameList(true); });

    // TV Standard
    connect(m_ui.actionTVNTSC, &QAction::triggered, this, &MainWindow::onTVStandardChanged);
    connect(m_ui.actionTVPAL, &QAction::triggered, this, &MainWindow::onTVStandardChanged);
    connect(m_ui.actionTVSECAM, &QAction::triggered, this, &MainWindow::onTVStandardChanged);

    // View menu
    connect(m_ui.actionViewToolbar, &QAction::toggled, this, &MainWindow::onViewToolbarActionToggled);
    connect(m_ui.actionViewLockToolbar, &QAction::toggled, this, &MainWindow::onViewLockToolbarActionToggled);
    connect(m_ui.actionViewStatusBar, &QAction::toggled, this, &MainWindow::onViewStatusBarActionToggled);
    connect(m_ui.actionViewGameList, &QAction::triggered, this, &MainWindow::onViewGameListActionTriggered);
    connect(m_ui.actionViewGameGrid, &QAction::triggered, this, &MainWindow::onViewGameGridActionTriggered);
    connect(m_ui.actionViewSystemDisplay, &QAction::triggered, this, &MainWindow::onViewSystemDisplayTriggered);
    connect(m_ui.actionFullscreen, &QAction::triggered, this, &MainWindow::onFullscreenActionTriggered);
    connect(m_ui.actionToolbarFullscreen, &QAction::triggered, this, &MainWindow::onFullscreenActionTriggered);

    // Grid view options
    connect(m_ui.actionGridViewShowTitles, &QAction::triggered, m_game_list_widget, &GameListWidget::setShowCoverTitles);
    connect(m_ui.actionGridViewZoomIn, &QAction::triggered, m_game_list_widget, [this]() {
        if (isShowingGameList())
            m_game_list_widget->gridZoomIn();
    });
    connect(m_ui.actionGridViewZoomOut, &QAction::triggered, m_game_list_widget, [this]() {
        if (isShowingGameList())
            m_game_list_widget->gridZoomOut();
    });
    connect(m_ui.actionGridViewRefreshCovers, &QAction::triggered, m_game_list_widget, &GameListWidget::refreshGridCovers);
    connect(m_game_list_widget, &GameListWidget::layoutChange, this, [this]() {
        QSignalBlocker sb(m_ui.actionGridViewShowTitles);
        m_ui.actionGridViewShowTitles->setChecked(m_game_list_widget->getShowGridCoverTitles());
        updateGameGridActions(m_game_list_widget->isShowingGameGrid());
    });

    // Help menu
    connect(m_ui.actionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(m_ui.actionAbout, &QAction::triggered, this, &MainWindow::onAboutActionTriggered);

    // Game list
    connect(m_game_list_widget, &GameListWidget::entryActivated, this, &MainWindow::onGameListEntryActivated);
    connect(m_game_list_widget, &GameListWidget::selectionChanged, this, &MainWindow::onGameListSelectionChanged);

    // Overlay menu
    connect(m_overlay_widget, &OverlayWidget::resumeClicked, this, [this]() {
        m_overlay_widget->hideOverlay();
        onPauseActionToggled(false);
    });
    connect(m_display_widget, &DisplayWidget::sizeChanged, this, [this](const QSize& sz) {
        if (m_overlay_widget)
            m_overlay_widget->resize(sz);
    });
    connect(m_overlay_widget, &OverlayWidget::resetClicked, this, [this]() {
        m_overlay_widget->hideOverlay();
        onPauseActionToggled(false);
        onResetActionTriggered();
    });
    connect(m_overlay_widget, &OverlayWidget::saveStateClicked, this, [this]() {
        saveSaveStateSlot(1); // Quick save to slot 1
    });
    connect(m_overlay_widget, &OverlayWidget::loadStateClicked, this, [this]() {
        loadSaveStateSlot(1); // Quick load from slot 1
    });
    connect(m_overlay_widget, &OverlayWidget::screenshotClicked, this, [this]() {
        onScreenshotActionTriggered();
    });
    connect(m_overlay_widget, &OverlayWidget::settingsClicked, this, [this]() {
        ControllerSettingsDialog dlg(m_sdl_input, this);
        dlg.exec();
    });
    connect(m_overlay_widget, &OverlayWidget::quitClicked, this, [this]() {
        m_overlay_widget->hideOverlay();
        onPowerOffActionTriggered();
    });

    // Emulation timer -> drives frame stepping
    connect(m_emulation_timer, &QTimer::timeout, this, [this]() {
        if (m_active_core && m_active_core->isRunning())
        {
            // Poll SDL input — use system-specific input when available
            if (m_jaguar_core && m_active_core == m_jaguar_core) {
                JaguarInputState js = m_sdl_input->pollJaguar();
                m_jaguar_core->setJaguarInputState(js);
            } else if (m_lynx_core && m_active_core == m_lynx_core) {
                LynxInputState js = m_sdl_input->pollLynx();
                m_lynx_core->setLynxInputState(js);
            } else {
                JoystickState js = m_sdl_input->poll();
                m_active_core->setJoystickState(js);
            }

            // Step one frame
            m_active_core->step();

            // Get frame and display it
            QImage frame = m_active_core->getFrame();
            if (!frame.isNull())
            {
                m_display_widget->updateFrame(frame);
            }

            // FPS counter
            m_fps_frame_count++;
            qint64 elapsed = m_fps_timer.elapsed();
            if (elapsed >= 1000)
            {
                double fps = m_fps_frame_count * 1000.0 / elapsed;
                double speed = fps / 60.0 * 100.0;
                m_status_fps_widget->setText(QString("%1 FPS (%2%)")
                    .arg(fps, 0, 'f', 1).arg(speed, 0, 'f', 0));
                m_status_fps_widget->show();
                m_fps_frame_count = 0;
                m_fps_timer.restart();
            }
        }
    });
}

// --- Slot implementations ---

void MainWindow::onStartFileActionTriggered()
{
    QString path = QFileDialog::getOpenFileName(this, tr("Start ROM"), QString(), tr(ROM_FILE_FILTER));
    if (path.isEmpty())
        return;
    startROM(path);
}

void MainWindow::onPowerOffActionTriggered()
{
    if (!m_is_running)
        return;
    stopEmulation();
    switchToGameListView();
}

void MainWindow::onResetActionTriggered()
{
    if (!m_active_core || !m_is_running)
        return;
    m_active_core->stop();
    m_active_core->loadROM(m_current_rom_path);
    m_active_core->start();
}

void MainWindow::onPauseActionToggled(bool checked)
{
    if (!m_is_running)
        return;

    auto activeConsoleName = [this]() -> QString {
        if (m_active_core == m_jaguar_core)    return QStringLiteral("Atari Jaguar");
        if (m_active_core == m_lynx_core)      return QStringLiteral("Atari Lynx");
        return QStringLiteral("Atari 2600");
    };

    if (checked)
    {
        m_emulation_timer->stop();
        m_status_verbose_widget->setText(tr("Paused"));
        if (m_discord && m_is_running)
            m_discord->updatePaused(m_current_game_title, activeConsoleName());
    }
    else
    {
        m_emulation_timer->start(16);
        m_status_verbose_widget->setText(tr("Running"));
        if (m_discord && m_is_running)
            m_discord->updatePlaying(m_current_game_title, activeConsoleName(), QString());
    }

    // Sync both menu and toolbar pause actions
    QSignalBlocker sb1(m_ui.actionPause);
    QSignalBlocker sb2(m_ui.actionToolbarPause);
    m_ui.actionPause->setChecked(checked);
    m_ui.actionToolbarPause->setChecked(checked);
}

void MainWindow::onToggleConsoleModeTriggered()
{
    ConsoleMode nextMode = ConsoleMode::Auto;

    if (m_console_mode == ConsoleMode::Auto)
        nextMode = ConsoleMode::Atari2600;
    else if (m_console_mode == ConsoleMode::Atari2600)
        nextMode = ConsoleMode::AtariLynx;
    else if (m_console_mode == ConsoleMode::AtariLynx)
        nextMode = ConsoleMode::AtariJaguar;

    setConsoleMode(nextMode);
}

void MainWindow::setConsoleMode(MainWindow::ConsoleMode mode)
{
    if (m_console_mode == mode)
        return;

    m_console_mode = mode;
    updateConsoleModeActionText();

    QString modeName;
    switch (mode)
    {
        case ConsoleMode::Auto:        modeName = tr("Auto");         break;
        case ConsoleMode::Atari2600:   modeName = tr("Atari 2600");   break;
        case ConsoleMode::AtariLynx:   modeName = tr("Atari Lynx");   break;
        case ConsoleMode::AtariJaguar: modeName = tr("Atari Jaguar"); break;
    }

    if (m_status_verbose_widget)
        m_status_verbose_widget->setText(tr("Mode: %1").arg(modeName));
    onStatusMessage(tr("Emulation mode set to %1").arg(modeName));
    updateWindowTitle();

    if (!m_current_rom_path.isEmpty())
    {
        QString romPath = m_current_rom_path;
        stopEmulation();
        startROM(romPath);
    }
}

void MainWindow::updateConsoleModeActionText()
{
    if (!m_toggle_console_mode_action)
        return;

    QString text;
    switch (m_console_mode)
    {
        case ConsoleMode::Auto:        text = tr("Console Mode: Auto");         break;
        case ConsoleMode::Atari2600:   text = tr("Console Mode: Atari 2600");   break;
        case ConsoleMode::AtariLynx:   text = tr("Console Mode: Atari Lynx");   break;
        case ConsoleMode::AtariJaguar: text = tr("Console Mode: Atari Jaguar"); break;
    }

    m_toggle_console_mode_action->setText(text);
}

QString MainWindow::getConsoleModeName() const
{
    switch (m_console_mode)
    {
        case ConsoleMode::Auto:        return tr("Auto");
        case ConsoleMode::Atari2600:   return tr("Atari 2600");
        case ConsoleMode::AtariLynx:   return tr("Atari Lynx");
        case ConsoleMode::AtariJaguar: return tr("Atari Jaguar");
    }
    return tr("Auto");
}

void MainWindow::onScreenshotActionTriggered()
{
    if (!m_active_core || !m_is_running)
        return;

    QImage frame = m_active_core->getFrame();
    if (frame.isNull())
        return;

    QString path = QFileDialog::getSaveFileName(this, tr("Save Screenshot"), QString(), tr("PNG Image (*.png)"));
    if (!path.isEmpty())
        frame.save(path);
}

void MainWindow::onFullscreenActionTriggered()
{
    if (isFullScreen())
    {
        showNormal();
        m_ui.toolBar->setVisible(m_ui.actionViewToolbar->isChecked());
        m_ui.statusBar->setVisible(m_ui.actionViewStatusBar->isChecked());
        // Menu bar stays hidden — gear button is in toolbar
    }
    else
    {
        m_ui.toolBar->setVisible(false);
        m_ui.statusBar->setVisible(false);
        showFullScreen();
    }
}

void MainWindow::onViewToolbarActionToggled(bool checked)
{
    m_ui.toolBar->setVisible(checked);
    QSettings settings;
    settings.setValue("UI/ShowToolbar", checked);
}

void MainWindow::onViewLockToolbarActionToggled(bool checked)
{
    m_ui.toolBar->setMovable(!checked);
    QSettings settings;
    settings.setValue("UI/LockToolbar", checked);
}

void MainWindow::onViewStatusBarActionToggled(bool checked)
{
    m_ui.statusBar->setVisible(checked);
    QSettings settings;
    settings.setValue("UI/ShowStatusBar", checked);
}

void MainWindow::onViewGameListActionTriggered()
{
    if (m_game_list_widget)
        m_game_list_widget->showGameList();
    QSettings settings;
    settings.setValue("UI/GameListGridView", false);
    updateGameGridActions(false);
}

void MainWindow::onViewGameGridActionTriggered()
{
    if (m_game_list_widget)
        m_game_list_widget->showGameGrid();
    QSettings settings;
    settings.setValue("UI/GameListGridView", true);
    updateGameGridActions(true);
}

void MainWindow::onViewSystemDisplayTriggered()
{
    if (m_is_running)
        switchToEmulationView();
}

void MainWindow::onAboutActionTriggered()
{
    QMessageBox::about(this, tr("About Íris Emulator"),
        tr("Íris Emulator\n\nAn Atari 2600 (and future Atari Lynx) emulator built with Qt and SDL2.\n"
           "Based on PCSX2 Qt interface design patterns."));
}

void MainWindow::onGameListEntryActivated()
{
    auto entry = m_game_list_widget->getSelectedEntry();
    if (!entry.has_value())
        return;

    const QString& path = entry.value();
    QSettings settings;
    QString onOpen = settings.value("UI/OnRomOpen", "start").toString();

    if (onOpen == "info")
    {
        QFileInfo fi(path);
        QString info = tr("<b>%1</b><br><br>"
                         "File: %2<br>"
                         "Size: %3 KB<br>"
                         "Path: %4")
            .arg(fi.completeBaseName())
            .arg(fi.fileName())
            .arg(fi.size() / 1024)
            .arg(fi.absolutePath());

        QMessageBox msgBox(this);
        msgBox.setWindowTitle(tr("Game Info"));
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setTextFormat(Qt::RichText);
        msgBox.setText(info);
        msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
        msgBox.button(QMessageBox::Ok)->setText(tr("Play"));
        msgBox.button(QMessageBox::Cancel)->setText(tr("Cancel"));

        if (msgBox.exec() != QMessageBox::Ok)
            return;
    }

    startROM(path);
}

void MainWindow::onGameListSelectionChanged()
{
    auto entry = m_game_list_widget->getSelectedEntry();
    if (entry.has_value())
    {
        QFileInfo fi(entry.value());
        m_status_verbose_widget->setText(fi.completeBaseName());
    }
}

void MainWindow::onLoadStateMenuAboutToShow()
{
    populateLoadStateMenu(m_ui.menuLoadState);
}

void MainWindow::onSaveStateMenuAboutToShow()
{
    populateSaveStateMenu(m_ui.menuSaveState);
}

void MainWindow::onTVStandardChanged()
{
    if (!m_emu_core)
        return;

    if (m_ui.actionTVNTSC->isChecked())
        m_emu_core->setTVStandard(TVStandard::NTSC);
    else if (m_ui.actionTVPAL->isChecked())
        m_emu_core->setTVStandard(TVStandard::PAL);
    else if (m_ui.actionTVSECAM->isChecked())
        m_emu_core->setTVStandard(TVStandard::SECAM);
}

void MainWindow::onSettingsTriggeredFromToolbar()
{
    QObject* s = sender();
    // If it's the controller settings action, open controller dialog directly
    if (s == m_ui.actionControllerSettings || s == m_ui.actionToolbarControllerSettings)
    {
        ControllerSettingsDialog dlg(m_sdl_input, this);
        dlg.exec();
        return;
    }
    // Open the full settings dialog
    SettingsDialog dlg(m_sdl_input, this);
    if (dlg.exec() == QDialog::Accepted)
    {
        // Apply all settings that changed
        applyVideoSettings();
        refreshGameList(true);

        // Sync TV standard with emulator core and toolbar actions
        QSettings settings;
        QString tvStd = settings.value("Video/TVStandard", "NTSC").toString();
        {
            QSignalBlocker sb1(m_ui.actionTVNTSC);
            QSignalBlocker sb2(m_ui.actionTVPAL);
            QSignalBlocker sb3(m_ui.actionTVSECAM);
            m_ui.actionTVNTSC->setChecked(tvStd == "NTSC");
            m_ui.actionTVPAL->setChecked(tvStd == "PAL");
            m_ui.actionTVSECAM->setChecked(tvStd == "SECAM");
        }
        if (m_emu_core)
        {
            if (tvStd == "PAL")
                m_emu_core->setTVStandard(TVStandard::PAL);
            else if (tvStd == "SECAM")
                m_emu_core->setTVStandard(TVStandard::SECAM);
            else
                m_emu_core->setTVStandard(TVStandard::NTSC);
        }

        // Apply VSync — adjust timer interval
        if (m_is_running && m_emulation_timer->isActive())
        {
            bool vsync = settings.value("Video/VSync", true).toBool();
            // With VSync: target ~60fps (16ms). Without: run as fast as possible (1ms).
            m_emulation_timer->setInterval(vsync ? 16 : 1);
        }

        // Apply audio settings
        if (m_active_core)
        {
            bool audioEnabled = settings.value("Audio/Enabled", true).toBool();
            int audioVolume = settings.value("Audio/Volume", 80).toInt();
            m_active_core->setAudioEnabled(audioEnabled);
            m_active_core->setAudioVolume(audioVolume);

            // If device changed and emulation is running, reinitialize audio
            if (m_is_running)
            {
                QString audioDevice = settings.value("Audio/Device", "default").toString();
                m_active_core->closeAudio();
                m_active_core->initAudio(audioDevice);
            }
        }
    }
}

void MainWindow::onAddGameDirectoryTriggered()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Add Game Directory"));
    if (dir.isEmpty())
        return;

    QSettings settings;
    QStringList dirs = settings.value("GameList/Directories").toStringList();
    if (!dirs.contains(dir))
    {
        dirs.append(dir);
        settings.setValue("GameList/Directories", dirs);
        refreshGameList(true);
    }
}

void MainWindow::onEmulationStarted()
{
    m_is_running = true;
    updateEmulationActions(true);
    switchToEmulationView();
    updateWindowTitle();
    m_status_verbose_widget->setText(tr("Running"));

    if (m_active_core == m_jaguar_core) {
        m_status_resolution_widget->setText(QStringLiteral("326x240"));
    } else if (m_active_core == m_lynx_core) {
        m_status_resolution_widget->setText(QStringLiteral("160x102"));
    } else {
        m_status_resolution_widget->setText(QStringLiteral("160x%1").arg(
            m_emu_core->getTVStandard() == TVStandard::NTSC ? 192 : 228));
    }
    m_status_resolution_widget->show();

    // Discord RPC
    if (m_discord) {
        QString console;
        if (m_active_core == m_jaguar_core)    console = "Atari Jaguar";
        else if (m_active_core == m_lynx_core) console = "Atari Lynx";
        else                                   console = "Atari 2600";
        m_discord->updatePlaying(m_current_game_title, console, QString());
    }
}

void MainWindow::onEmulationStopped()
{
    m_is_running = false;
    updateEmulationActions(false);
    updateWindowTitle();
    m_status_verbose_widget->setText(tr("Ready"));
    m_status_fps_widget->hide();
    m_status_resolution_widget->hide();

    if (m_discord)
        m_discord->updateMenu();
}

void MainWindow::onFrameReady(const QImage& frame)
{
    if (m_display_widget)
        m_display_widget->updateFrame(frame);
}

// --- View switching (PCSX2 pattern) ---

bool MainWindow::isShowingGameList() const
{
    return m_ui.mainContainer->currentWidget() == m_game_list_widget;
}

void MainWindow::switchToGameListView()
{
    if (isShowingGameList())
    {
        m_game_list_widget->setFocus();
        return;
    }

    m_ui.mainContainer->setCurrentWidget(m_game_list_widget);
    m_ui.actionViewSystemDisplay->setEnabled(m_is_running);
}

void MainWindow::switchToEmulationView()
{
    if (!m_display_widget)
        return;

    m_ui.mainContainer->setCurrentWidget(m_display_widget);
    m_display_widget->setFocus();
    m_ui.actionViewSystemDisplay->setEnabled(true);
}

// --- Emulation control ---

void MainWindow::startROM(const QString& path)
{
    // Stop any existing emulation
    stopEmulation();

    QString lower = path.toLower();
    bool isLynxPath   = lower.endsWith(".lnx") || lower.endsWith(".lyx");
    bool isJaguarPath = lower.endsWith(".j64") || lower.endsWith(".jag");
    ConsoleMode actualConsole = m_console_mode;

    if (actualConsole == ConsoleMode::Auto) {
        if (isJaguarPath)      actualConsole = ConsoleMode::AtariJaguar;
        else if (isLynxPath)   actualConsole = ConsoleMode::AtariLynx;
        else                   actualConsole = ConsoleMode::Atari2600;
    }

    if (actualConsole == ConsoleMode::AtariJaguar) {
        if (m_active_core == m_emu_core) { delete m_emu_core; m_emu_core = new EmulatorCore(this); }
        if (m_active_core == m_lynx_core) { delete m_lynx_core; m_lynx_core = nullptr; }
        if (!m_jaguar_core)
            m_jaguar_core = new JaguarSystem(this);
        m_active_core = m_jaguar_core;
    } else if (actualConsole == ConsoleMode::AtariLynx) {
        if (m_active_core == m_emu_core) { delete m_emu_core; m_emu_core = new EmulatorCore(this); }
        if (m_active_core == m_jaguar_core) { delete m_jaguar_core; m_jaguar_core = nullptr; }
        if (!m_lynx_core)
            m_lynx_core = new LynxSystem(this);
        m_active_core = m_lynx_core;
    } else {
        if (m_active_core == m_lynx_core) { delete m_lynx_core; m_lynx_core = nullptr; }
        if (m_active_core == m_jaguar_core) { delete m_jaguar_core; m_jaguar_core = nullptr; }
        m_active_core = m_emu_core;
    }

    if (!m_active_core->loadROM(path))
    {
        QMessageBox::critical(this, tr("Error"), tr("Failed to load ROM: %1").arg(path));
        m_active_core = nullptr;
        return;
    }

    // Aviso de BIOS faltando para o Lynx
    if (actualConsole == ConsoleMode::AtariLynx) {
        auto* lynx = qobject_cast<LynxSystem*>(m_active_core);
        if (lynx && !lynx->biosFound()) {
            QMessageBox warn(this);
            warn.setWindowTitle(tr("Atari Lynx — Boot ROM not found"));
            warn.setIcon(QMessageBox::Warning);
            warn.setText(tr(
                "<b>The Lynx Boot ROM (lynxboot.img) was not found.</b><br><br>"
                "Most commercial games require the original boot ROM to decrypt and run correctly.<br>"
                "Without it, games may show a black screen or crash.<br><br>"
                "<b>We cannot provide the BIOS file</b> as it is copyrighted by Atari.<br>"
                "You must obtain it from your own Atari Lynx hardware.<br><br>"
                "Place <code>lynxboot.img</code> in the same folder as the emulator,<br>"
                "or configure the path in <b>Settings &rarr; Lynx Controls &rarr; Boot ROM file</b>."));
            warn.setStandardButtons(QMessageBox::Ok | QMessageBox::Open);
            warn.button(QMessageBox::Open)->setText(tr("Open Settings"));
            if (warn.exec() == QMessageBox::Open) {
                SettingsDialog dlg(m_sdl_input, this);
                dlg.exec();
            }
        }
    }

    m_current_rom_path = path;
    m_current_game_title = QFileInfo(path).completeBaseName();

    // Apply current TV standard (2600 only)
    if (actualConsole == ConsoleMode::Atari2600)
        onTVStandardChanged();

    m_active_core->start();

    // Initialize audio output
    {
        QSettings audioSettings;
        bool audioEnabled = audioSettings.value("Audio/Enabled", true).toBool();
        QString audioDevice = audioSettings.value("Audio/Device", "default").toString();
        int audioVolume = audioSettings.value("Audio/Volume", 80).toInt();

        m_active_core->setAudioEnabled(audioEnabled);
        m_active_core->setAudioVolume(audioVolume);
        m_active_core->initAudio(audioDevice);
    }

    // Start the frame timer — respect VSync setting
    QSettings timerSettings;
    bool vsync = timerSettings.value("Video/VSync", true).toBool();
    m_emulation_timer->start(vsync ? 16 : 1); // VSync: ~60fps, No VSync: uncapped
    m_fps_frame_count = 0;
    m_fps_timer.restart();

    // Sync pause state
    QSignalBlocker sb1(m_ui.actionPause);
    QSignalBlocker sb2(m_ui.actionToolbarPause);
    m_ui.actionPause->setChecked(false);
    m_ui.actionToolbarPause->setChecked(false);

    onEmulationStarted();
}

void MainWindow::stopEmulation()
{
    if (m_emulation_timer)
        m_emulation_timer->stop();

    if (m_active_core)
    {
        m_active_core->stop();
        m_active_core->closeAudio();
    }

    m_active_core = nullptr;
    m_current_rom_path.clear();
    m_current_game_title.clear();

    onEmulationStopped();
}

void MainWindow::applyVideoSettings()
{
    QSettings settings;
    QString filterStr = settings.value("Video/Filter", "none").toString();
    QString lynxFilterStr = settings.value("Video/LynxFilter", "smooth").toString();

    // Escolhe o filtro certo baseado no console ativo
    bool isLynx = (m_active_core == m_lynx_core);
    QString activeFilter = isLynx ? lynxFilterStr : filterStr;

    auto strToFilter = [](const QString& s) {
        if (s == "scanlines") return DisplayWidget::ScreenFilter::Scanlines;
        if (s == "crt")       return DisplayWidget::ScreenFilter::CRT;
        if (s == "smooth")    return DisplayWidget::ScreenFilter::Smooth;
        if (s == "lcd")       return DisplayWidget::ScreenFilter::LCD;
        if (s == "lcdblur")   return DisplayWidget::ScreenFilter::LCDBlur;
        return DisplayWidget::ScreenFilter::None;
    };

    m_display_widget->setScreenFilter(strToFilter(activeFilter));
    m_display_widget->setScanlineIntensity(settings.value("Video/ScanlineIntensity", 50).toInt());
    m_display_widget->setLCDGhostingStrength(settings.value("Video/LCDGhosting", 40).toInt());

    bool vsync = settings.value("Video/VSync", true).toBool();
    if (m_is_running && m_emulation_timer->isActive())
        m_emulation_timer->setInterval(vsync ? 16 : 1);

    if (m_emu_core && m_is_running && !isLynx) {
        QString tvStd = settings.value("Video/TVStandard", "NTSC").toString();
        if (tvStd == "PAL")        m_emu_core->setTVStandard(TVStandard::PAL);
        else if (tvStd == "SECAM") m_emu_core->setTVStandard(TVStandard::SECAM);
        else                       m_emu_core->setTVStandard(TVStandard::NTSC);
        m_status_resolution_widget->setText(QStringLiteral("160x%1").arg(
            m_emu_core->getTVStandard() == TVStandard::NTSC ? 192 : 228));
    }
}

// --- State management ---

void MainWindow::updateEmulationActions(bool running)
{
    m_ui.actionPowerOff->setEnabled(running);
    m_ui.actionToolbarPowerOff->setEnabled(running);
    m_ui.actionReset->setEnabled(running);
    m_ui.actionToolbarReset->setEnabled(running);
    m_ui.actionPause->setEnabled(running);
    m_ui.actionToolbarPause->setEnabled(running);
    m_ui.actionScreenshot->setEnabled(running);
    m_ui.actionToolbarScreenshot->setEnabled(running);
    m_ui.actionToolbarLoadState->setEnabled(running);
    m_ui.actionToolbarSaveState->setEnabled(running);
    m_ui.actionViewSystemDisplay->setEnabled(running);
}

void MainWindow::updateGameGridActions(bool show_game_grid)
{
    m_ui.actionGridViewShowTitles->setEnabled(show_game_grid);
    m_ui.actionGridViewZoomIn->setEnabled(show_game_grid);
    m_ui.actionGridViewZoomOut->setEnabled(show_game_grid);
}

void MainWindow::updateWindowTitle()
{
    QString title = QStringLiteral("Íris Emulator");
    if (!m_current_game_title.isEmpty())
    {
        title += QStringLiteral(" - ") + m_current_game_title;
        title += QStringLiteral(" (") + getConsoleModeName() + QStringLiteral(")");
        if (m_ui.actionPause->isChecked())
            title += QStringLiteral(" [Paused]");
    }
    else
    {
        title += QStringLiteral(" - ") + getConsoleModeName();
    }
    setWindowTitle(title);
}

void MainWindow::saveStateToConfig()
{
    QSettings settings;
    settings.setValue("MainWindow/Geometry", saveGeometry());
    settings.setValue("MainWindow/State", saveState());
}

void MainWindow::restoreStateFromConfig()
{
    QSettings settings;
    if (settings.contains("MainWindow/Geometry"))
        restoreGeometry(settings.value("MainWindow/Geometry").toByteArray());
    if (settings.contains("MainWindow/State"))
        restoreState(settings.value("MainWindow/State").toByteArray());
}

void MainWindow::refreshGameList(bool invalidate_cache)
{
    if (m_game_list_widget)
        m_game_list_widget->refresh(invalidate_cache);
}

void MainWindow::onStatusMessage(const QString& message)
{
    m_ui.statusBar->showMessage(message, 3000);
}

void MainWindow::setProgressBar(int current, int total)
{
    if (total > 0)
    {
        m_status_progress_widget->setMaximum(total);
        m_status_progress_widget->setValue(current);
        m_status_progress_widget->show();
    }
}

void MainWindow::clearProgressBar()
{
    m_status_progress_widget->hide();
}

// --- Save / Load state slots ---

void MainWindow::loadSaveStateSlot(int slot)
{
    if (!m_active_core || m_current_rom_path.isEmpty())
        return;

    QString state_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/savestates";
    QString basename = QFileInfo(m_current_rom_path).completeBaseName();
    QString state_path = state_dir + "/" + basename + QString(".slot%1.sav").arg(slot);

    if (!QFile::exists(state_path))
    {
        onStatusMessage(tr("No save state in slot %1").arg(slot));
        return;
    }

    if (m_active_core->loadState(state_path))
        onStatusMessage(tr("State loaded from slot %1").arg(slot));
    else
        onStatusMessage(tr("Failed to load state from slot %1").arg(slot));
}

void MainWindow::saveSaveStateSlot(int slot)
{
    if (!m_active_core || m_current_rom_path.isEmpty())
        return;

    QString state_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/savestates";
    QDir().mkpath(state_dir);
    QString basename = QFileInfo(m_current_rom_path).completeBaseName();
    QString state_path = state_dir + "/" + basename + QString(".slot%1.sav").arg(slot);

    if (m_active_core->saveState(state_path))
        onStatusMessage(tr("State saved to slot %1").arg(slot));
    else
        onStatusMessage(tr("Failed to save state to slot %1").arg(slot));
}

void MainWindow::populateLoadStateMenu(QMenu* menu)
{
    menu->clear();

    if (m_current_rom_path.isEmpty())
        return;

    QString state_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/savestates";
    QString basename = QFileInfo(m_current_rom_path).completeBaseName();

    for (int slot = 1; slot <= 10; slot++)
    {
        QString state_path = state_dir + "/" + basename + QString(".slot%1.sav").arg(slot);
        QString label;
        if (QFile::exists(state_path))
        {
            QFileInfo fi(state_path);
            label = tr("Slot %1 - %2").arg(slot).arg(fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm")));
        }
        else
        {
            label = tr("Slot %1 - Empty").arg(slot);
        }

        QAction* action = menu->addAction(label);
        action->setEnabled(QFile::exists(state_path));
        connect(action, &QAction::triggered, [this, slot]() { loadSaveStateSlot(slot); });
    }
}

void MainWindow::populateSaveStateMenu(QMenu* menu)
{
    menu->clear();

    if (m_current_rom_path.isEmpty())
        return;

    QString state_dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/savestates";
    QString basename = QFileInfo(m_current_rom_path).completeBaseName();

    for (int slot = 1; slot <= 10; slot++)
    {
        QString state_path = state_dir + "/" + basename + QString(".slot%1.sav").arg(slot);
        QString label;
        if (QFile::exists(state_path))
        {
            QFileInfo fi(state_path);
            label = tr("Slot %1 - %2").arg(slot).arg(fi.lastModified().toString(QStringLiteral("yyyy-MM-dd hh:mm")));
        }
        else
        {
            label = tr("Slot %1 - Empty").arg(slot);
        }

        QAction* action = menu->addAction(label);
        connect(action, &QAction::triggered, [this, slot]() { saveSaveStateSlot(slot); });
    }
}

// --- Events ---

void MainWindow::closeEvent(QCloseEvent* event)
{
    stopEmulation();
    saveStateToConfig();
    event->accept();
}

void MainWindow::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        for (const QUrl& url : event->mimeData()->urls())
        {
            QString path = url.toLocalFile().toLower();
            if (path.endsWith(".bin") || path.endsWith(".a26") || path.endsWith(".rom")
                || path.endsWith(".lnx") || path.endsWith(".lyx")
                || path.endsWith(".j64") || path.endsWith(".jag"))
            {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dropEvent(QDropEvent* event)
{
    if (event->mimeData()->hasUrls())
    {
        for (const QUrl& url : event->mimeData()->urls())
        {
            QString path = url.toLocalFile();
            QString lower = path.toLower();
            if (lower.endsWith(".bin") || lower.endsWith(".a26") || lower.endsWith(".rom")
                || lower.endsWith(".lnx") || lower.endsWith(".lyx")
                || lower.endsWith(".j64") || lower.endsWith(".jag"))
            {
                startROM(path);
                return;
            }
        }
    }
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape && m_is_running)
    {
        if (m_overlay_widget->isOverlayVisible())
        {
            // Resume
            m_overlay_widget->hideOverlay();
            onPauseActionToggled(false);
            m_display_widget->setFocus();
        }
        else
        {
            // Pause + show overlay
            onPauseActionToggled(true);
            m_overlay_widget->resize(m_display_widget->size());
            m_overlay_widget->showOverlay();
        }
        return;
    }
    QMainWindow::keyPressEvent(event);
}
