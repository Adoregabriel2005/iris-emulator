#include <QApplication>
#include <QDebug>
#include <QFile>
#include "EmulatorCore.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc < 2) {
        qDebug() << "Usage: test_savestate <path-to-rom> [frames_pre=5] [frames_post=3]";
        return 2;
    }
    QString rom = QString::fromUtf8(argv[1]);
    int frames_pre = 5;
    int frames_post = 3;
    if (argc >= 3) frames_pre = QString::fromUtf8(argv[2]).toInt();
    if (argc >= 4) frames_post = QString::fromUtf8(argv[3]).toInt();

    EmulatorCore core;
    if (!core.loadROM(rom)) {
        qDebug() << "Failed to load ROM:" << rom;
        return 3;
    }

    core.start();
    for (int i = 0; i < frames_pre; ++i) core.step();

    QByteArray state = core.dumpState();
    qDebug() << "Saved state size:" << state.size();

    for (int i = 0; i < frames_post; ++i) core.step();

    if (!core.loadStateFromData(state)) {
        qDebug() << "Failed to reload state from memory";
        return 4;
    }

    QByteArray state2 = core.dumpState();
    qDebug() << "Reloaded state size:" << state2.size();

    if (state == state2) {
        qDebug() << "PASS: state round-trip identical";
        return 0;
    }

    qDebug() << "FAIL: state differs";
    int n = qMin(state.size(), state2.size());
    int diff = -1;
    for (int i = 0; i < n; ++i) {
        if (state[i] != state2[i]) { diff = i; break; }
    }
    if (diff >= 0) qDebug() << "First diff at byte" << diff << "saved=" << (uint8_t)state[diff] << "loaded=" << (uint8_t)state2[diff];

    // write files for inspection
    QFile f1("saved_state.bin"); if (f1.open(QIODevice::WriteOnly)) f1.write(state);
    QFile f2("reloaded_state.bin"); if (f2.open(QIODevice::WriteOnly)) f2.write(state2);

    return 1;
}
