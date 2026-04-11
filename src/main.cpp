#include <QApplication>

// Required by Gearlynx log.h
bool g_mcp_stdio_mode = false;
#include <QIcon>
#include <QLocale>
#include <QSettings>
#include <QTimer>
#include <QTranslator>
#include "ui/MainWindow.h"
#include "ui/Themes.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Íris Emulator");
    app.setOrganizationName("IrisEmulator");
    app.setWindowIcon(QIcon(":/iris_icon.svg"));

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

    MainWindow w;
    w.initialize();
    w.show();

    // Load ROM from command line if provided
    QStringList args = app.arguments();
    if (args.size() > 1) {
        QString romPath = args.at(1);
        QTimer::singleShot(100, [&w, romPath]() {
            w.loadROMFromCommandLine(romPath);
        });
    }

    return app.exec();
}
