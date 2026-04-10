#include "MapperFactory.h"
#include "mapper/MapperNone.h"
#include "mapper/MapperBanked.h"
#include "mapper/MapperF8.h"
#include "mapper/MapperF6.h"
#include "mapper/MapperF4.h"
#include "mapper/MapperE0.h"
#include "mapper/MapperE7.h"
#include "mapper/MapperSC.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QCoreApplication>
#include <QDebug>

std::unique_ptr<Mapper> MapperFactory::createMapperForROM(const QByteArray *rom, const QString &sha1, const QString &appDir)
{
    // Try to load data/mappers.json inside appDir or current working directory
    QString cfgPath = appDir + "/data/mappers.json";
    QFile f(cfgPath);
    if (!f.exists()) {
        // try relative to cwd
        cfgPath = QCoreApplication::applicationDirPath() + "/../data/mappers.json";
        f.setFileName(cfgPath);
    }
    if (f.exists() && f.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        if (doc.isObject()) {
            QJsonObject root = doc.object();
            if (!sha1.isEmpty() && root.contains(sha1)) {
                QJsonObject ent = root.value(sha1).toObject();
                QString type = ent.value("type").toString();
                // Named mapper types
                if (type == "F8")   return std::make_unique<MapperF8>(rom);
                if (type == "F8SC") return std::make_unique<MapperF8SC>(rom);
                if (type == "F6")   return std::make_unique<MapperF6>(rom);
                if (type == "F6SC") return std::make_unique<MapperF6SC>(rom);
                if (type == "F4")   return std::make_unique<MapperF4>(rom);
                if (type == "F4SC") return std::make_unique<MapperF4SC>(rom);
                if (type == "E0")   return std::make_unique<MapperE0>(rom);
                if (type == "E7")   return std::make_unique<MapperE7>(rom);
                if (type == "none") return std::make_unique<MapperNone>(rom);
                if (type == "banked") {
                    int bankSize = ent.value("bank_size").toInt(0x1000);
                    std::vector<uint16_t> controlAddrs;
                    QJsonArray carr = ent.value("control_addrs").toArray();
                    for (auto v : carr) {
                        QString s = v.toString();
                        bool ok = false;
                        uint16_t a = static_cast<uint16_t>(s.toUInt(&ok, 0));
                        if (ok) controlAddrs.push_back(a);
                    }
                    return std::make_unique<MapperBanked>(rom, bankSize, controlAddrs);
                }
            }
        }
    }

    // Default heuristics
    int size = rom ? rom->size() : 0;
    if (size <= 0x1000) {
        return std::make_unique<MapperNone>(rom);
    }

    // Detect Superchip: look for accesses to $1000-$10FF in ROM
    bool hasSC = false;
    if (rom) {
        const char* d = rom->constData();
        int scCount = 0;
        for (int i = 0; i < size - 1; i++) {
            uint8_t lo = static_cast<uint8_t>(d[i]);
            uint8_t hi = static_cast<uint8_t>(d[i+1]);
            // Look for addresses $1000-$10FF (STA $10xx or LDA $10xx)
            if (hi == 0x10 && lo <= 0xFF) {
                // Check if previous byte is a load/store opcode
                if (i > 0) {
                    uint8_t op = static_cast<uint8_t>(d[i-1]);
                    // Common absolute addressing opcodes: LDA=0xAD, STA=0x8D, LDX=0xAE, STX=0x8E, LDY=0xAC, STY=0x8C
                    if (op == 0xAD || op == 0x8D || op == 0xAE || op == 0x8E || op == 0xAC || op == 0x8C) {
                        scCount++;
                    }
                }
            }
        }
        hasSC = (scCount >= 8);
    }

    // Heuristics by common ROM sizes
    if (size == 0x8000) { // 32KB -> F4 (8 banks) or F4SC
        if (hasSC) return std::make_unique<MapperF4SC>(rom);
        return std::make_unique<MapperF4>(rom);
    }
    if (size == 0x4000) { // 16KB -> F6 (4 banks) or F6SC
        if (hasSC) return std::make_unique<MapperF6SC>(rom);
        return std::make_unique<MapperF6>(rom);
    }
    if (size == 0x2000) { // 8KB -> F8 (2 banks) or E0 (Parker Brothers) or F8SC
        // Detect E0: check for accesses to $1FE0-$1FF7 hotspots in ROM data
        if (rom) {
            int e0Count = 0;
            const char* d = rom->constData();
            for (int i = 0; i < size - 2; i++) {
                // Look for addresses in the $xFE0-$xFF7 range (high byte could vary)
                uint8_t lo = static_cast<uint8_t>(d[i]);
                uint8_t hi = static_cast<uint8_t>(d[i+1]);
                if ((hi & 0x1F) == 0x1F && lo >= 0xE0 && lo <= 0xF7) {
                    e0Count++;
                }
            }
            if (e0Count >= 4) {
                return std::make_unique<MapperE0>(rom);
            }
        }
        if (hasSC) return std::make_unique<MapperF8SC>(rom);
        return std::make_unique<MapperF8>(rom);
    }

    // default: use 4KB banks with some common control addresses as heuristic
    std::vector<uint16_t> ctrl = { 0x1FF8, 0x1FF9, 0x1FFA, 0x1FFB };
    return std::make_unique<MapperBanked>(rom, 0x1000, ctrl);
}
