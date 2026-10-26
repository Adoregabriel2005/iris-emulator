#include "SDLInput.h"
#include <QCoreApplication>
#include <QSettings>
#include <QTemporaryDir>
#include <QSet>
#include <QtCore/Qt>
#include <cstdio>
#include <cstdlib>

static void check(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    app.setOrganizationName("IrisInputRegression");
    app.setApplicationName("JaguarInput");
    QTemporaryDir config;
    check(config.isValid(), "temporary settings directory");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, config.path());

    QSet<QString> names;
    for (int i = 0; i < static_cast<int>(InputAction::Count); ++i) {
        const auto action = static_cast<InputAction>(i);
        const QString name = SDLInput::actionName(action);
        check(name != "Unknown" && !names.contains(name), "each action has an independent settings key");
        names.insert(name);
        check(SDLInput::defaultBinding(action).isValid(), "each action has a default binding");
    }

    SDLInput input;
    struct Key { int qt; bool JaguarInputState::*field; };
    const Key keys[] = {
        {Qt::Key_Up, &JaguarInputState::up}, {Qt::Key_Down, &JaguarInputState::down},
        {Qt::Key_Left, &JaguarInputState::left}, {Qt::Key_Right, &JaguarInputState::right},
        {Qt::Key_Z, &JaguarInputState::a}, {Qt::Key_X, &JaguarInputState::b}, {Qt::Key_C, &JaguarInputState::c},
        {Qt::Key_O, &JaguarInputState::option}, {Qt::Key_Return, &JaguarInputState::pause},
        {Qt::Key_1, &JaguarInputState::n1}, {Qt::Key_2, &JaguarInputState::n2}, {Qt::Key_3, &JaguarInputState::n3},
        {Qt::Key_4, &JaguarInputState::n4}, {Qt::Key_5, &JaguarInputState::n5}, {Qt::Key_6, &JaguarInputState::n6},
        {Qt::Key_7, &JaguarInputState::n7}, {Qt::Key_8, &JaguarInputState::n8}, {Qt::Key_9, &JaguarInputState::n9},
        {Qt::Key_Asterisk, &JaguarInputState::star}, {Qt::Key_0, &JaguarInputState::n0},
        {Qt::Key_NumberSign, &JaguarInputState::hash}
    };
    for (const auto& key : keys) {
        input.setQtKeyState(key.qt, true);
        const auto state = input.pollJaguarKeyboard();
        for (const auto& other : keys)
            check(state.*(other.field) == (key.field == other.field), "Qt key activates only its Jaguar button");
        input.setQtKeyState(key.qt, false);
        check(!(input.pollJaguarKeyboard().*(key.field)), "Qt key release reaches Jaguar");
    }
    InputBinding custom{InputBinding::Type::Keyboard, SDL_SCANCODE_Q, 0};
    input.setBinding(InputAction::JagA, custom);
    input.saveBindings();
    input.resetToDefaults();
    input.loadBindings();
    check(input.getBinding(InputAction::JagA).code == SDL_SCANCODE_Q, "custom Jaguar A binding persists");
    check(input.getBinding(InputAction::JagB).code == SDL_SCANCODE_X, "saving Jaguar A cannot overwrite Jaguar B");
    input.setBinding(InputAction::JagA, {InputBinding::Type::Keyboard, SDL_NUM_SCANCODES + 1, 0});
    check(!input.pollJaguarKeyboard().a, "invalid stored keyboard binding is ignored");
    std::puts("PASS: Jaguar names, defaults, 21 Qt buttons, release and independent binding persistence");
}
