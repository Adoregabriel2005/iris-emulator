/********************************************************************************
** Form generated from reading UI file 'MainWindow.ui'
**
** Created by: Qt User Interface Compiler version 6.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAINWINDOW_H
#define UI_MAINWINDOW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QIcon>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStackedWidget>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>

QT_BEGIN_NAMESPACE

class Ui_MainWindow
{
public:
    QAction *actionStartFile;
    QAction *actionToolbarStartFile;
    QAction *actionPowerOff;
    QAction *actionToolbarPowerOff;
    QAction *actionReset;
    QAction *actionToolbarReset;
    QAction *actionPause;
    QAction *actionToolbarPause;
    QAction *actionScreenshot;
    QAction *actionToolbarScreenshot;
    QAction *actionToolbarLoadState;
    QAction *actionToolbarSaveState;
    QAction *actionExit;
    QAction *actionEmulationSettings;
    QAction *actionAudioSettings;
    QAction *actionControllerSettings;
    QAction *actionToolbarControllerSettings;
    QAction *actionToolbarSettings;
    QAction *actionScanForNewGames;
    QAction *actionRescanAllGames;
    QAction *actionAddGameDirectory;
    QAction *actionViewToolbar;
    QAction *actionViewLockToolbar;
    QAction *actionViewStatusBar;
    QAction *actionViewGameList;
    QAction *actionViewGameGrid;
    QAction *actionViewSystemDisplay;
    QAction *actionFullscreen;
    QAction *actionToolbarFullscreen;
    QAction *actionGridViewShowTitles;
    QAction *actionGridViewZoomIn;
    QAction *actionGridViewZoomOut;
    QAction *actionGridViewRefreshCovers;
    QAction *actionAboutQt;
    QAction *actionAbout;
    QAction *actionTVNTSC;
    QAction *actionTVPAL;
    QAction *actionTVSECAM;
    QStackedWidget *mainContainer;
    QMenuBar *menuBar;
    QMenu *menuSystem;
    QMenu *menuLoadState;
    QMenu *menuSaveState;
    QMenu *menuSettings;
    QMenu *menuTVStandard;
    QMenu *menuView;
    QMenu *menuWindowSize;
    QMenu *menuHelp;
    QToolBar *toolBar;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *MainWindow)
    {
        if (MainWindow->objectName().isEmpty())
            MainWindow->setObjectName("MainWindow");
        MainWindow->resize(1050, 666);
        MainWindow->setAcceptDrops(true);
        MainWindow->setUnifiedTitleAndToolBarOnMac(true);
        actionStartFile = new QAction(MainWindow);
        actionStartFile->setObjectName("actionStartFile");
        QIcon icon(QIcon::fromTheme(QString::fromUtf8("file-line")));
        actionStartFile->setIcon(icon);
        actionToolbarStartFile = new QAction(MainWindow);
        actionToolbarStartFile->setObjectName("actionToolbarStartFile");
        actionToolbarStartFile->setIcon(icon);
        actionPowerOff = new QAction(MainWindow);
        actionPowerOff->setObjectName("actionPowerOff");
        QIcon icon1(QIcon::fromTheme(QString::fromUtf8("shut-down-line")));
        actionPowerOff->setIcon(icon1);
        actionToolbarPowerOff = new QAction(MainWindow);
        actionToolbarPowerOff->setObjectName("actionToolbarPowerOff");
        actionToolbarPowerOff->setIcon(icon1);
        actionReset = new QAction(MainWindow);
        actionReset->setObjectName("actionReset");
        QIcon icon2(QIcon::fromTheme(QString::fromUtf8("restart-line")));
        actionReset->setIcon(icon2);
        actionToolbarReset = new QAction(MainWindow);
        actionToolbarReset->setObjectName("actionToolbarReset");
        actionToolbarReset->setIcon(icon2);
        actionPause = new QAction(MainWindow);
        actionPause->setObjectName("actionPause");
        actionPause->setCheckable(true);
        QIcon icon3(QIcon::fromTheme(QString::fromUtf8("pause-line")));
        actionPause->setIcon(icon3);
        actionToolbarPause = new QAction(MainWindow);
        actionToolbarPause->setObjectName("actionToolbarPause");
        actionToolbarPause->setCheckable(true);
        actionToolbarPause->setIcon(icon3);
        actionScreenshot = new QAction(MainWindow);
        actionScreenshot->setObjectName("actionScreenshot");
        QIcon icon4(QIcon::fromTheme(QString::fromUtf8("screenshot-2-line")));
        actionScreenshot->setIcon(icon4);
        actionToolbarScreenshot = new QAction(MainWindow);
        actionToolbarScreenshot->setObjectName("actionToolbarScreenshot");
        actionToolbarScreenshot->setIcon(icon4);
        actionToolbarLoadState = new QAction(MainWindow);
        actionToolbarLoadState->setObjectName("actionToolbarLoadState");
        QIcon icon5(QIcon::fromTheme(QString::fromUtf8("floppy-out-line")));
        actionToolbarLoadState->setIcon(icon5);
        actionToolbarSaveState = new QAction(MainWindow);
        actionToolbarSaveState->setObjectName("actionToolbarSaveState");
        QIcon icon6(QIcon::fromTheme(QString::fromUtf8("floppy-in-line")));
        actionToolbarSaveState->setIcon(icon6);
        actionExit = new QAction(MainWindow);
        actionExit->setObjectName("actionExit");
        QIcon icon7(QIcon::fromTheme(QString::fromUtf8("door-open-line")));
        actionExit->setIcon(icon7);
        actionEmulationSettings = new QAction(MainWindow);
        actionEmulationSettings->setObjectName("actionEmulationSettings");
        QIcon icon8(QIcon::fromTheme(QString::fromUtf8("settings-3-line")));
        actionEmulationSettings->setIcon(icon8);
        actionAudioSettings = new QAction(MainWindow);
        actionAudioSettings->setObjectName("actionAudioSettings");
        QIcon icon9(QIcon::fromTheme(QString::fromUtf8("volume-up-line")));
        actionAudioSettings->setIcon(icon9);
        actionControllerSettings = new QAction(MainWindow);
        actionControllerSettings->setObjectName("actionControllerSettings");
        QIcon icon10(QIcon::fromTheme(QString::fromUtf8("controller-line")));
        actionControllerSettings->setIcon(icon10);
        actionToolbarControllerSettings = new QAction(MainWindow);
        actionToolbarControllerSettings->setObjectName("actionToolbarControllerSettings");
        actionToolbarControllerSettings->setIcon(icon10);
        actionToolbarSettings = new QAction(MainWindow);
        actionToolbarSettings->setObjectName("actionToolbarSettings");
        actionToolbarSettings->setIcon(icon8);
        actionScanForNewGames = new QAction(MainWindow);
        actionScanForNewGames->setObjectName("actionScanForNewGames");
        QIcon icon11(QIcon::fromTheme(QString::fromUtf8("file-search-line")));
        actionScanForNewGames->setIcon(icon11);
        actionRescanAllGames = new QAction(MainWindow);
        actionRescanAllGames->setObjectName("actionRescanAllGames");
        QIcon icon12(QIcon::fromTheme(QString::fromUtf8("refresh-line")));
        actionRescanAllGames->setIcon(icon12);
        actionAddGameDirectory = new QAction(MainWindow);
        actionAddGameDirectory->setObjectName("actionAddGameDirectory");
        QIcon icon13(QIcon::fromTheme(QString::fromUtf8("folder-add-line")));
        actionAddGameDirectory->setIcon(icon13);
        actionViewToolbar = new QAction(MainWindow);
        actionViewToolbar->setObjectName("actionViewToolbar");
        actionViewToolbar->setCheckable(true);
        actionViewToolbar->setChecked(true);
        actionViewLockToolbar = new QAction(MainWindow);
        actionViewLockToolbar->setObjectName("actionViewLockToolbar");
        actionViewLockToolbar->setCheckable(true);
        actionViewLockToolbar->setChecked(false);
        actionViewStatusBar = new QAction(MainWindow);
        actionViewStatusBar->setObjectName("actionViewStatusBar");
        actionViewStatusBar->setCheckable(true);
        actionViewStatusBar->setChecked(true);
        actionViewGameList = new QAction(MainWindow);
        actionViewGameList->setObjectName("actionViewGameList");
        QIcon icon14(QIcon::fromTheme(QString::fromUtf8("list-check")));
        actionViewGameList->setIcon(icon14);
        actionViewGameGrid = new QAction(MainWindow);
        actionViewGameGrid->setObjectName("actionViewGameGrid");
        QIcon icon15(QIcon::fromTheme(QString::fromUtf8("function-line")));
        actionViewGameGrid->setIcon(icon15);
        actionViewSystemDisplay = new QAction(MainWindow);
        actionViewSystemDisplay->setObjectName("actionViewSystemDisplay");
        actionViewSystemDisplay->setEnabled(false);
        QIcon icon16(QIcon::fromTheme(QString::fromUtf8("tv-2-line")));
        actionViewSystemDisplay->setIcon(icon16);
        actionFullscreen = new QAction(MainWindow);
        actionFullscreen->setObjectName("actionFullscreen");
        QIcon icon17(QIcon::fromTheme(QString::fromUtf8("fullscreen-line")));
        actionFullscreen->setIcon(icon17);
        actionToolbarFullscreen = new QAction(MainWindow);
        actionToolbarFullscreen->setObjectName("actionToolbarFullscreen");
        actionToolbarFullscreen->setIcon(icon17);
        actionGridViewShowTitles = new QAction(MainWindow);
        actionGridViewShowTitles->setObjectName("actionGridViewShowTitles");
        actionGridViewShowTitles->setCheckable(true);
        actionGridViewZoomIn = new QAction(MainWindow);
        actionGridViewZoomIn->setObjectName("actionGridViewZoomIn");
        QIcon icon18(QIcon::fromTheme(QString::fromUtf8("zoom-in-line")));
        actionGridViewZoomIn->setIcon(icon18);
        actionGridViewZoomOut = new QAction(MainWindow);
        actionGridViewZoomOut->setObjectName("actionGridViewZoomOut");
        QIcon icon19(QIcon::fromTheme(QString::fromUtf8("zoom-out-line")));
        actionGridViewZoomOut->setIcon(icon19);
        actionGridViewRefreshCovers = new QAction(MainWindow);
        actionGridViewRefreshCovers->setObjectName("actionGridViewRefreshCovers");
        actionGridViewRefreshCovers->setIcon(icon12);
        actionAboutQt = new QAction(MainWindow);
        actionAboutQt->setObjectName("actionAboutQt");
        actionAbout = new QAction(MainWindow);
        actionAbout->setObjectName("actionAbout");
        actionTVNTSC = new QAction(MainWindow);
        actionTVNTSC->setObjectName("actionTVNTSC");
        actionTVNTSC->setCheckable(true);
        actionTVNTSC->setChecked(true);
        actionTVPAL = new QAction(MainWindow);
        actionTVPAL->setObjectName("actionTVPAL");
        actionTVPAL->setCheckable(true);
        actionTVSECAM = new QAction(MainWindow);
        actionTVSECAM->setObjectName("actionTVSECAM");
        actionTVSECAM->setCheckable(true);
        mainContainer = new QStackedWidget(MainWindow);
        mainContainer->setObjectName("mainContainer");
        MainWindow->setCentralWidget(mainContainer);
        menuBar = new QMenuBar(MainWindow);
        menuBar->setObjectName("menuBar");
        menuBar->setGeometry(QRect(0, 0, 1050, 27));
        menuSystem = new QMenu(menuBar);
        menuSystem->setObjectName("menuSystem");
        menuLoadState = new QMenu(menuSystem);
        menuLoadState->setObjectName("menuLoadState");
        menuLoadState->setIcon(icon5);
        menuSaveState = new QMenu(menuSystem);
        menuSaveState->setObjectName("menuSaveState");
        menuSaveState->setIcon(icon6);
        menuSettings = new QMenu(menuBar);
        menuSettings->setObjectName("menuSettings");
        menuTVStandard = new QMenu(menuSettings);
        menuTVStandard->setObjectName("menuTVStandard");
        menuTVStandard->setIcon(icon16);
        menuView = new QMenu(menuBar);
        menuView->setObjectName("menuView");
        menuWindowSize = new QMenu(menuView);
        menuWindowSize->setObjectName("menuWindowSize");
        QIcon icon20(QIcon::fromTheme(QString::fromUtf8("window-2-line")));
        menuWindowSize->setIcon(icon20);
        menuHelp = new QMenu(menuBar);
        menuHelp->setObjectName("menuHelp");
        MainWindow->setMenuBar(menuBar);
        toolBar = new QToolBar(MainWindow);
        toolBar->setObjectName("toolBar");
        toolBar->setVisible(true);
        toolBar->setIconSize(QSize(32, 32));
        toolBar->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextUnderIcon);
        MainWindow->addToolBar(Qt::ToolBarArea::TopToolBarArea, toolBar);
        statusBar = new QStatusBar(MainWindow);
        statusBar->setObjectName("statusBar");
        MainWindow->setStatusBar(statusBar);

        menuBar->addAction(menuSystem->menuAction());
        menuBar->addAction(menuSettings->menuAction());
        menuBar->addAction(menuView->menuAction());
        menuBar->addAction(menuHelp->menuAction());
        menuSystem->addAction(actionStartFile);
        menuSystem->addSeparator();
        menuSystem->addAction(actionPowerOff);
        menuSystem->addAction(actionReset);
        menuSystem->addAction(actionPause);
        menuSystem->addSeparator();
        menuSystem->addAction(actionScreenshot);
        menuSystem->addSeparator();
        menuSystem->addAction(menuLoadState->menuAction());
        menuSystem->addAction(menuSaveState->menuAction());
        menuSystem->addSeparator();
        menuSystem->addAction(actionExit);
        menuSettings->addAction(actionEmulationSettings);
        menuSettings->addAction(actionAudioSettings);
        menuSettings->addAction(actionControllerSettings);
        menuSettings->addSeparator();
        menuSettings->addAction(menuTVStandard->menuAction());
        menuSettings->addSeparator();
        menuSettings->addAction(actionAddGameDirectory);
        menuSettings->addAction(actionScanForNewGames);
        menuSettings->addAction(actionRescanAllGames);
        menuTVStandard->addAction(actionTVNTSC);
        menuTVStandard->addAction(actionTVPAL);
        menuTVStandard->addAction(actionTVSECAM);
        menuView->addAction(actionViewToolbar);
        menuView->addAction(actionViewLockToolbar);
        menuView->addAction(actionViewStatusBar);
        menuView->addSeparator();
        menuView->addAction(actionViewGameList);
        menuView->addAction(actionViewGameGrid);
        menuView->addAction(actionViewSystemDisplay);
        menuView->addSeparator();
        menuView->addAction(actionFullscreen);
        menuView->addAction(menuWindowSize->menuAction());
        menuView->addSeparator();
        menuView->addAction(actionGridViewShowTitles);
        menuView->addAction(actionGridViewZoomIn);
        menuView->addAction(actionGridViewZoomOut);
        menuView->addAction(actionGridViewRefreshCovers);
        menuHelp->addAction(actionAboutQt);
        menuHelp->addAction(actionAbout);
        toolBar->addAction(actionToolbarStartFile);
        toolBar->addSeparator();
        toolBar->addAction(actionToolbarPowerOff);
        toolBar->addAction(actionToolbarReset);
        toolBar->addAction(actionToolbarPause);
        toolBar->addSeparator();
        toolBar->addAction(actionToolbarScreenshot);
        toolBar->addSeparator();
        toolBar->addAction(actionToolbarLoadState);
        toolBar->addAction(actionToolbarSaveState);
        toolBar->addSeparator();
        toolBar->addAction(actionToolbarFullscreen);
        toolBar->addSeparator();
        toolBar->addAction(actionToolbarSettings);
        toolBar->addAction(actionToolbarControllerSettings);

        retranslateUi(MainWindow);

        QMetaObject::connectSlotsByName(MainWindow);
    } // setupUi

    void retranslateUi(QMainWindow *MainWindow)
    {
        MainWindow->setWindowTitle(QCoreApplication::translate("MainWindow", "\303\215ris Emulator", nullptr));
        actionStartFile->setText(QCoreApplication::translate("MainWindow", "Start &File...", nullptr));
        actionToolbarStartFile->setText(QCoreApplication::translate("MainWindow", "Start File", "In Toolbar"));
        actionPowerOff->setText(QCoreApplication::translate("MainWindow", "Shut &Down", nullptr));
        actionToolbarPowerOff->setText(QCoreApplication::translate("MainWindow", "Shut Down", "In Toolbar"));
        actionReset->setText(QCoreApplication::translate("MainWindow", "&Reset", nullptr));
        actionToolbarReset->setText(QCoreApplication::translate("MainWindow", "Reset", "In Toolbar"));
        actionPause->setText(QCoreApplication::translate("MainWindow", "&Pause", nullptr));
        actionToolbarPause->setText(QCoreApplication::translate("MainWindow", "Pause", "In Toolbar"));
        actionScreenshot->setText(QCoreApplication::translate("MainWindow", "&Screenshot", nullptr));
        actionToolbarScreenshot->setText(QCoreApplication::translate("MainWindow", "Screenshot", "In Toolbar"));
        actionToolbarLoadState->setText(QCoreApplication::translate("MainWindow", "Load State", "In Toolbar"));
        actionToolbarSaveState->setText(QCoreApplication::translate("MainWindow", "Save State", "In Toolbar"));
        actionExit->setText(QCoreApplication::translate("MainWindow", "E&xit", nullptr));
        actionEmulationSettings->setText(QCoreApplication::translate("MainWindow", "&Emulation", nullptr));
        actionAudioSettings->setText(QCoreApplication::translate("MainWindow", "&Audio", nullptr));
        actionControllerSettings->setText(QCoreApplication::translate("MainWindow", "&Controllers", nullptr));
        actionToolbarControllerSettings->setText(QCoreApplication::translate("MainWindow", "Controllers", "In Toolbar"));
        actionToolbarSettings->setText(QCoreApplication::translate("MainWindow", "Settings", "In Toolbar"));
        actionScanForNewGames->setText(QCoreApplication::translate("MainWindow", "&Scan For New Games", nullptr));
        actionRescanAllGames->setText(QCoreApplication::translate("MainWindow", "&Rescan All Games", nullptr));
        actionAddGameDirectory->setText(QCoreApplication::translate("MainWindow", "Add Game &Directory...", nullptr));
        actionViewToolbar->setText(QCoreApplication::translate("MainWindow", "&Toolbar", nullptr));
        actionViewLockToolbar->setText(QCoreApplication::translate("MainWindow", "Loc&k Toolbar", nullptr));
        actionViewStatusBar->setText(QCoreApplication::translate("MainWindow", "&Status Bar", nullptr));
        actionViewGameList->setText(QCoreApplication::translate("MainWindow", "Game &List", nullptr));
        actionViewGameGrid->setText(QCoreApplication::translate("MainWindow", "Game &Grid", nullptr));
        actionViewSystemDisplay->setText(QCoreApplication::translate("MainWindow", "System &Display", nullptr));
        actionFullscreen->setText(QCoreApplication::translate("MainWindow", "&Fullscreen", nullptr));
        actionToolbarFullscreen->setText(QCoreApplication::translate("MainWindow", "Fullscreen", "In Toolbar"));
        actionGridViewShowTitles->setText(QCoreApplication::translate("MainWindow", "Show &Titles", nullptr));
        actionGridViewZoomIn->setText(QCoreApplication::translate("MainWindow", "Zoom &In", nullptr));
        actionGridViewZoomOut->setText(QCoreApplication::translate("MainWindow", "Zoom &Out", nullptr));
        actionGridViewRefreshCovers->setText(QCoreApplication::translate("MainWindow", "&Refresh Covers", nullptr));
        actionAboutQt->setText(QCoreApplication::translate("MainWindow", "About &Qt...", nullptr));
        actionAbout->setText(QCoreApplication::translate("MainWindow", "&About \303\215ris Emulator...", nullptr));
        actionTVNTSC->setText(QCoreApplication::translate("MainWindow", "&NTSC", nullptr));
        actionTVPAL->setText(QCoreApplication::translate("MainWindow", "&PAL", nullptr));
        actionTVSECAM->setText(QCoreApplication::translate("MainWindow", "&SECAM", nullptr));
        menuSystem->setTitle(QCoreApplication::translate("MainWindow", "&System", nullptr));
        menuLoadState->setTitle(QCoreApplication::translate("MainWindow", "&Load State", nullptr));
        menuSaveState->setTitle(QCoreApplication::translate("MainWindow", "Sa&ve State", nullptr));
        menuSettings->setTitle(QCoreApplication::translate("MainWindow", "Setti&ngs", nullptr));
        menuTVStandard->setTitle(QCoreApplication::translate("MainWindow", "&TV Standard", nullptr));
        menuView->setTitle(QCoreApplication::translate("MainWindow", "&View", nullptr));
        menuWindowSize->setTitle(QCoreApplication::translate("MainWindow", "&Window Size", nullptr));
        menuHelp->setTitle(QCoreApplication::translate("MainWindow", "&Help", nullptr));
        toolBar->setWindowTitle(QCoreApplication::translate("MainWindow", "Toolbar", nullptr));
    } // retranslateUi

};

namespace Ui {
    class MainWindow: public Ui_MainWindow {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAINWINDOW_H
