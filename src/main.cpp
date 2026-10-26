#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSaveFile>

// Required by Gearlynx log.h
bool g_mcp_stdio_mode = false;
#include <QIcon>
#include <QLocale>
#include <QSettings>
#include <QTimer>
#include <QTranslator>
#include "ui/MainWindow.h"
#include "ui/Themes.h"
#include "ui/EmulatorLogWindow.h"

int main(int argc, char *argv[])
{
    bool smokeTest = argc > 2 && QByteArray(argv[1]) == "--smoke-test";
    if (smokeTest) qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    app.setApplicationName("Íris Emulator");
    app.setApplicationVersion(QStringLiteral(IRIS_VERSION));
    app.setOrganizationName("IrisEmulator");
    app.setWindowIcon(QIcon(":/iris_icon.svg"));

    const QStringList arguments = app.arguments();
    QString smokeOutput;
    int targetFrames = 600;
    if (smokeTest) {
        const int outputIndex = arguments.indexOf("--output");
        const int framesIndex = arguments.indexOf("--frames");
        if (outputIndex < 0 || outputIndex + 1 >= arguments.size()) return 2;
        smokeOutput = QFileInfo(arguments.at(outputIndex + 1)).absoluteFilePath();
        app.setProperty("smokeOutput", smokeOutput);
        app.setProperty("smokeAudio", arguments.contains("--audio-test"));
        if (framesIndex >= 0 && framesIndex + 1 < arguments.size())
            targetFrames = arguments.at(framesIndex + 1).toInt();
        if (targetFrames < 1 || targetFrames > 36000 || !QDir().mkpath(smokeOutput)) return 2;
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, smokeOutput + "/config");
        QSettings testSettings;
        testSettings.setValue("GameList/Directories", QStringList{smokeOutput});
        testSettings.setValue("Audio/Enabled", arguments.contains("--audio-test"));
        testSettings.setValue("Discord/Enabled", false);
        testSettings.setValue("Video/VSync", false);
        testSettings.setValue("Jaguar/EEPROMPath", smokeOutput + "/eeprom/");
        if (arguments.contains("--pal")) testSettings.setValue("Jaguar/TVStandard", "PAL");
    }

    // Load translation based on language setting
    QSettings settings;
    QString lang = settings.value("UI/Language", "en").toString();
    QTranslator appTranslator;
    if (lang != "en")
    {
        // Try to load from app directory first, then Qt translations
        QString translationDir = QApplication::applicationDirPath() + "/translations";
        if (appTranslator.load("atari2600_" + lang, translationDir))
            app.installTranslator(&appTranslator);
    }
    // Also load Qt's own translated strings (buttons, dialogs, etc.)
    QTranslator qtTranslator;
    if (lang != "en")
    {
        if (qtTranslator.load("qt_" + lang, QApplication::applicationDirPath() + "/translations"))
            app.installTranslator(&qtTranslator);
    }

    // Initialize icon theme search paths for Qt resource icons
    QStringList iconPaths = QIcon::themeSearchPaths();
    iconPaths.prepend(":/icons");
    QIcon::setThemeSearchPaths(iconPaths);

    // Apply default dark theme (PCSX2 pattern)
    Themes::UpdateApplicationTheme();

    EmulatorLogWindow::installHandler();

    MainWindow w;
    w.setProperty("smokeTest", smokeTest);
    w.setProperty("smokePaced", smokeTest && arguments.contains("--paced"));
    if (smokeTest && arguments.contains("--input-script")) {
        const int index = arguments.indexOf("--input-script");
        if (index + 1 >= arguments.size()) return 2;
        QFile inputFile(arguments.at(index + 1));
        if (!inputFile.open(QIODevice::ReadOnly)) return 2;
        const auto document = QJsonDocument::fromJson(inputFile.readAll());
        if (!document.isArray()) return 2;
        w.setProperty("smokeInputScript", document.array().toVariantList());
    }
    w.initialize();
    w.show();

    int completedFrames = 0;
    qint64 firstFrameMs = -1;
    QElapsedTimer smokeElapsed;
    smokeElapsed.start();
    auto writeSmokeResult = [&](const QString& status, const QString& reason = QString()) {
        QJsonObject result{{"rom", arguments.value(2)}, {"status", status},
            {"frames", completedFrames}, {"targetFrames", targetFrames},
            {"presentedFrames", w.property("presentedFrames").toInt()},
            {"elapsedMs", double(smokeElapsed.elapsed())}, {"reason", reason},
            {"firstFrameMs", double(firstFrameMs)},
            {"audioTested", arguments.contains("--audio-test")},
            {"audio", QJsonObject::fromVariantMap(w.property("audioDiagnostics").toMap())},
            {"ramSeedOverride", qEnvironmentVariable("IRIS_JAGUAR_RAM_SEED")},
            {"paced", arguments.contains("--paced")},
            {"inputScript", QJsonArray::fromVariantList(w.property("smokeInputScript").toList())},
            {"frontend", "Qt offscreen, real MainWindow frame loop"}};
        QSaveFile file(smokeOutput + "/result.json");
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(result).toJson());
            file.commit();
        }
    };
    if (smokeTest) {
        writeSmokeResult("starting");
        QObject::connect(&w, &MainWindow::emulationFrameCompleted, &app, [&](const QImage& frame) {
            ++completedFrames;
            if (completedFrames == 1) firstFrameMs = smokeElapsed.elapsed();
            const int resetIndex = arguments.indexOf("--reset-at");
            if (resetIndex >= 0 && completedFrames == arguments.value(resetIndex + 1).toInt())
                QMetaObject::invokeMethod(&w, "onResetActionTriggered", Qt::QueuedConnection);
            if (completedFrames == 1 || completedFrames % 300 == 0 || completedFrames == targetFrames) {
                frame.save(smokeOutput + QString("/frame-%1.png").arg(completedFrames));
                writeSmokeResult("running");
            }
            if (completedFrames == targetFrames) {
                writeSmokeResult("completed");
                QTimer::singleShot(0, &app, &QCoreApplication::quit);
            }
        });
        QObject::connect(&w, &MainWindow::emulationFailed, &app, [&](const QString& reason) {
            writeSmokeResult("core_stopped", reason);
            app.exit(3);
        });
        QTimer::singleShot(120000, &app, [&]() {
            writeSmokeResult("timeout", "Frontend did not complete the requested frames within 120s");
            app.exit(4);
        });
    }

    // Load ROM from command line if provided
    QStringList args = app.arguments();
    if (args.size() > 1) {
        QString romPath = args.at(smokeTest ? 2 : 1);
        QTimer::singleShot(100, [&w, romPath]() {
            w.loadROMFromCommandLine(romPath);
        });
    }

    return app.exec();
}
