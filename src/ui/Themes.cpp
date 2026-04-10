#include "Themes.h"

#include <QtCore/QSettings>
#include <QtGui/QPalette>
#include <QtGui/QPixmapCache>
#include <QtWidgets/QApplication>
#include <QtWidgets/QStyle>
#include <QtWidgets/QStyleFactory>

static QString s_unthemed_style_name;
static QPalette s_unthemed_palette;
static bool s_unthemed_style_name_set = false;

const char* Themes::GetDefaultThemeName()
{
    return "darkfusionblue";
}

void Themes::UpdateApplicationTheme()
{
    if (!s_unthemed_style_name_set)
    {
        s_unthemed_style_name_set = true;
        s_unthemed_style_name = QApplication::style()->objectName();
        s_unthemed_palette = QApplication::palette();
    }

    QSettings settings;
    const QString theme = settings.value("UI/Theme", GetDefaultThemeName()).toString();

    if (theme == "fusion")
    {
        qApp->setStyle(QStyleFactory::create("Fusion"));
        qApp->setPalette(s_unthemed_palette);
        qApp->setStyleSheet(QString());
    }
    else if (theme == "darkfusion")
    {
        qApp->setStyle(QStyleFactory::create("Fusion"));

        const QColor lighterGray(75, 75, 75);
        const QColor darkGray(53, 53, 53);
        const QColor gray(128, 128, 128);
        const QColor black(25, 25, 25);
        const QColor blue(198, 238, 255);

        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, darkGray);
        darkPalette.setColor(QPalette::WindowText, Qt::white);
        darkPalette.setColor(QPalette::Base, black);
        darkPalette.setColor(QPalette::AlternateBase, darkGray);
        darkPalette.setColor(QPalette::ToolTipBase, darkGray);
        darkPalette.setColor(QPalette::ToolTipText, Qt::white);
        darkPalette.setColor(QPalette::Text, Qt::white);
        darkPalette.setColor(QPalette::Button, darkGray);
        darkPalette.setColor(QPalette::ButtonText, Qt::white);
        darkPalette.setColor(QPalette::Link, blue);
        darkPalette.setColor(QPalette::Highlight, lighterGray);
        darkPalette.setColor(QPalette::HighlightedText, Qt::white);
        darkPalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

        darkPalette.setColor(QPalette::Active, QPalette::Button, darkGray);
        darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
        darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
        darkPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
        darkPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

        qApp->setPalette(darkPalette);
        qApp->setStyleSheet(QString());
    }
    else if (theme == "darkfusionblue")
    {
        qApp->setStyle(QStyleFactory::create("Fusion"));

        const QColor darkGray(53, 53, 53);
        const QColor gray(128, 128, 128);
        const QColor black(25, 25, 25);
        const QColor blue(198, 238, 255);
        const QColor blue2(0, 88, 208);

        QPalette darkBluePalette;
        darkBluePalette.setColor(QPalette::Window, darkGray);
        darkBluePalette.setColor(QPalette::WindowText, Qt::white);
        darkBluePalette.setColor(QPalette::Base, black);
        darkBluePalette.setColor(QPalette::AlternateBase, darkGray);
        darkBluePalette.setColor(QPalette::ToolTipBase, blue2);
        darkBluePalette.setColor(QPalette::ToolTipText, Qt::white);
        darkBluePalette.setColor(QPalette::Text, Qt::white);
        darkBluePalette.setColor(QPalette::Button, darkGray);
        darkBluePalette.setColor(QPalette::ButtonText, Qt::white);
        darkBluePalette.setColor(QPalette::Link, blue);
        darkBluePalette.setColor(QPalette::Highlight, blue2);
        darkBluePalette.setColor(QPalette::HighlightedText, Qt::white);
        darkBluePalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

        darkBluePalette.setColor(QPalette::Active, QPalette::Button, darkGray);
        darkBluePalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
        darkBluePalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
        darkBluePalette.setColor(QPalette::Disabled, QPalette::Text, gray);
        darkBluePalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

        qApp->setPalette(darkBluePalette);
        qApp->setStyleSheet(QStringLiteral(
            "QToolBar { border-bottom: 1px solid #1a1a1a; spacing: 2px; padding: 2px; }"
            "QToolBar QToolButton { padding: 4px 6px; border-radius: 4px; }"
            "QToolBar QToolButton:hover { background-color: rgba(0, 88, 208, 100); }"
            "QStatusBar { border-top: 1px solid #1a1a1a; font-size: 12px; }"
            "QStatusBar QLabel { padding: 0 6px; }"
            "QMenuBar { border-bottom: 1px solid #1a1a1a; }"
            "QMenuBar::item:selected { background-color: rgba(0, 88, 208, 150); border-radius: 3px; }"
            "QMenu { border: 1px solid #444; }"
            "QMenu::item:selected { background-color: rgba(0, 88, 208, 180); }"
            "QTableView { gridline-color: #333; }"
            "QTableView::item:selected { background-color: rgba(0, 88, 208, 150); }"
            "QHeaderView::section { background-color: #2a2a2a; border: none; border-right: 1px solid #444; "
            "  border-bottom: 1px solid #444; padding: 4px 8px; }"
            "QScrollBar:vertical { width: 10px; background: transparent; }"
            "QScrollBar::handle:vertical { background: #555; border-radius: 5px; min-height: 20px; }"
            "QScrollBar::handle:vertical:hover { background: #777; }"
            "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
            "QScrollBar:horizontal { height: 10px; background: transparent; }"
            "QScrollBar::handle:horizontal { background: #555; border-radius: 5px; min-width: 20px; }"
            "QScrollBar::handle:horizontal:hover { background: #777; }"
            "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }"
        ));
    }
    else if (theme == "GreyMatter")
    {
        qApp->setStyle(QStyleFactory::create("Fusion"));

        const QColor darkGray(46, 52, 64);
        const QColor lighterGray(59, 66, 82);
        const QColor gray(111, 111, 111);
        const QColor blue(198, 238, 255);

        QPalette greyMatterPalette;
        greyMatterPalette.setColor(QPalette::Window, darkGray);
        greyMatterPalette.setColor(QPalette::WindowText, Qt::white);
        greyMatterPalette.setColor(QPalette::Base, lighterGray);
        greyMatterPalette.setColor(QPalette::AlternateBase, darkGray);
        greyMatterPalette.setColor(QPalette::ToolTipBase, darkGray);
        greyMatterPalette.setColor(QPalette::ToolTipText, Qt::white);
        greyMatterPalette.setColor(QPalette::Text, Qt::white);
        greyMatterPalette.setColor(QPalette::Button, lighterGray);
        greyMatterPalette.setColor(QPalette::ButtonText, Qt::white);
        greyMatterPalette.setColor(QPalette::Link, blue);
        greyMatterPalette.setColor(QPalette::Highlight, lighterGray.lighter());
        greyMatterPalette.setColor(QPalette::HighlightedText, Qt::white);
        greyMatterPalette.setColor(QPalette::PlaceholderText, QColor(Qt::white).darker());

        greyMatterPalette.setColor(QPalette::Active, QPalette::Button, lighterGray);
        greyMatterPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray.lighter());
        greyMatterPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray.lighter());
        greyMatterPalette.setColor(QPalette::Disabled, QPalette::Text, gray.lighter());
        greyMatterPalette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

        qApp->setPalette(greyMatterPalette);
        qApp->setStyleSheet(QString());
    }
    else if (theme == "AMOLED")
    {
        qApp->setStyle(QStyleFactory::create("Fusion"));

        const QColor black(0, 0, 0);
        const QColor darkGray(20, 20, 20);
        const QColor gray(128, 128, 128);
        const QColor blue(0, 150, 255);
        const QColor white(Qt::white);

        QPalette amoledPalette;
        amoledPalette.setColor(QPalette::Window, black);
        amoledPalette.setColor(QPalette::WindowText, white);
        amoledPalette.setColor(QPalette::Base, darkGray);
        amoledPalette.setColor(QPalette::AlternateBase, black);
        amoledPalette.setColor(QPalette::ToolTipBase, darkGray);
        amoledPalette.setColor(QPalette::ToolTipText, white);
        amoledPalette.setColor(QPalette::Text, white);
        amoledPalette.setColor(QPalette::Button, darkGray);
        amoledPalette.setColor(QPalette::ButtonText, white);
        amoledPalette.setColor(QPalette::Link, blue);
        amoledPalette.setColor(QPalette::Highlight, blue);
        amoledPalette.setColor(QPalette::HighlightedText, white);
        amoledPalette.setColor(QPalette::PlaceholderText, gray);

        amoledPalette.setColor(QPalette::Active, QPalette::Button, darkGray);
        amoledPalette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
        amoledPalette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
        amoledPalette.setColor(QPalette::Disabled, QPalette::Text, gray);
        amoledPalette.setColor(QPalette::Disabled, QPalette::Light, black);

        qApp->setPalette(amoledPalette);
        qApp->setStyleSheet(QString());
    }

    SetIconThemeFromStyle();
    QPixmapCache::clear();
}

bool Themes::IsDarkApplicationTheme()
{
    QPalette palette = qApp->palette();
    return palette.windowText().color().value() > palette.window().color().value();
}

void Themes::SetIconThemeFromStyle()
{
    const bool dark = IsDarkApplicationTheme();
    QIcon::setThemeName(dark ? QStringLiteral("white") : QStringLiteral("black"));
}
