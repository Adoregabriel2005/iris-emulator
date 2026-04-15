#pragma once

#include <QtWidgets/QApplication>

namespace Themes
{
    const char* GetDefaultThemeName();
    void UpdateApplicationTheme();
    bool IsDarkApplicationTheme();
    void SetIconThemeFromStyle();
}
