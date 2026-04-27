/****************************************************************************
** Meta object code from reading C++ file 'MainWindow.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../../src/ui/MainWindow.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'MainWindow.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN10MainWindowE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN10MainWindowE = QtMocHelpers::stringData(
    "MainWindow",
    "refreshGameList",
    "",
    "invalidate_cache",
    "onStatusMessage",
    "message",
    "onStartFileActionTriggered",
    "onViewToolbarActionToggled",
    "checked",
    "onViewLockToolbarActionToggled",
    "onViewStatusBarActionToggled",
    "onViewGameListActionTriggered",
    "onViewGameGridActionTriggered",
    "onViewSystemDisplayTriggered",
    "onAboutActionTriggered",
    "onGameListEntryActivated",
    "onGameListSelectionChanged",
    "onPowerOffActionTriggered",
    "onResetActionTriggered",
    "onPauseActionToggled",
    "onScreenshotActionTriggered",
    "onFullscreenActionTriggered",
    "onLoadStateMenuAboutToShow",
    "onSaveStateMenuAboutToShow",
    "onTVStandardChanged",
    "onSettingsTriggeredFromToolbar",
    "onToggleConsoleModeTriggered",
    "onAddGameDirectoryTriggered",
    "onEmulationStarted",
    "onEmulationStopped",
    "onFrameReady",
    "frame"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN10MainWindowE[] = {

 // content:
      12,       // revision
       0,       // classname
       0,    0, // classinfo
      26,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,  170,    2, 0x0a,    1 /* Public */,
       4,    1,  173,    2, 0x0a,    3 /* Public */,
       6,    0,  176,    2, 0x08,    5 /* Private */,
       7,    1,  177,    2, 0x08,    6 /* Private */,
       9,    1,  180,    2, 0x08,    8 /* Private */,
      10,    1,  183,    2, 0x08,   10 /* Private */,
      11,    0,  186,    2, 0x08,   12 /* Private */,
      12,    0,  187,    2, 0x08,   13 /* Private */,
      13,    0,  188,    2, 0x08,   14 /* Private */,
      14,    0,  189,    2, 0x08,   15 /* Private */,
      15,    0,  190,    2, 0x08,   16 /* Private */,
      16,    0,  191,    2, 0x08,   17 /* Private */,
      17,    0,  192,    2, 0x08,   18 /* Private */,
      18,    0,  193,    2, 0x08,   19 /* Private */,
      19,    1,  194,    2, 0x08,   20 /* Private */,
      20,    0,  197,    2, 0x08,   22 /* Private */,
      21,    0,  198,    2, 0x08,   23 /* Private */,
      22,    0,  199,    2, 0x08,   24 /* Private */,
      23,    0,  200,    2, 0x08,   25 /* Private */,
      24,    0,  201,    2, 0x08,   26 /* Private */,
      25,    0,  202,    2, 0x08,   27 /* Private */,
      26,    0,  203,    2, 0x08,   28 /* Private */,
      27,    0,  204,    2, 0x08,   29 /* Private */,
      28,    0,  205,    2, 0x08,   30 /* Private */,
      29,    0,  206,    2, 0x08,   31 /* Private */,
      30,    1,  207,    2, 0x08,   32 /* Private */,

 // slots: parameters
    QMetaType::Void, QMetaType::Bool,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Bool,    8,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QImage,   31,

       0        // eod
};

Q_CONSTINIT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_ZN10MainWindowE.offsetsAndSizes,
    qt_meta_data_ZN10MainWindowE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN10MainWindowE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<MainWindow, std::true_type>,
        // method 'refreshGameList'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onStatusMessage'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QString &, std::false_type>,
        // method 'onStartFileActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onViewToolbarActionToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onViewLockToolbarActionToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onViewStatusBarActionToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onViewGameListActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onViewGameGridActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onViewSystemDisplayTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAboutActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGameListEntryActivated'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onGameListSelectionChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPowerOffActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onResetActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onPauseActionToggled'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<bool, std::false_type>,
        // method 'onScreenshotActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFullscreenActionTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onLoadStateMenuAboutToShow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSaveStateMenuAboutToShow'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onTVStandardChanged'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onSettingsTriggeredFromToolbar'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onToggleConsoleModeTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onAddGameDirectoryTriggered'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onEmulationStarted'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onEmulationStopped'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        // method 'onFrameReady'
        QtPrivate::TypeAndForceComplete<void, std::false_type>,
        QtPrivate::TypeAndForceComplete<const QImage &, std::false_type>
    >,
    nullptr
} };

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->refreshGameList((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 1: _t->onStatusMessage((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 2: _t->onStartFileActionTriggered(); break;
        case 3: _t->onViewToolbarActionToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 4: _t->onViewLockToolbarActionToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 5: _t->onViewStatusBarActionToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 6: _t->onViewGameListActionTriggered(); break;
        case 7: _t->onViewGameGridActionTriggered(); break;
        case 8: _t->onViewSystemDisplayTriggered(); break;
        case 9: _t->onAboutActionTriggered(); break;
        case 10: _t->onGameListEntryActivated(); break;
        case 11: _t->onGameListSelectionChanged(); break;
        case 12: _t->onPowerOffActionTriggered(); break;
        case 13: _t->onResetActionTriggered(); break;
        case 14: _t->onPauseActionToggled((*reinterpret_cast< std::add_pointer_t<bool>>(_a[1]))); break;
        case 15: _t->onScreenshotActionTriggered(); break;
        case 16: _t->onFullscreenActionTriggered(); break;
        case 17: _t->onLoadStateMenuAboutToShow(); break;
        case 18: _t->onSaveStateMenuAboutToShow(); break;
        case 19: _t->onTVStandardChanged(); break;
        case 20: _t->onSettingsTriggeredFromToolbar(); break;
        case 21: _t->onToggleConsoleModeTriggered(); break;
        case 22: _t->onAddGameDirectoryTriggered(); break;
        case 23: _t->onEmulationStarted(); break;
        case 24: _t->onEmulationStopped(); break;
        case 25: _t->onFrameReady((*reinterpret_cast< std::add_pointer_t<QImage>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN10MainWindowE.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 26)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 26;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 26)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 26;
    }
    return _id;
}
QT_WARNING_POP
