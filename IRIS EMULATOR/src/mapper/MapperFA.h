#pragma once
#include "Mapper.h"

// CBS FA mapper (RAM Plus)
// 12KB ROM in 3x4KB banks + 256 bytes RAM
// Hotspots: $1FF8=bank0, $1FF9=bank1, $1FFA=bank2
// RAM: write $1000-$10FF, read $1100-$11FF
class MapperFA : public Mapper {
public:
    explicit MapperFA(const QByteArray *romData)
        : Mapper(romData), currentBank(2)
    {
        memset(ram, 0, sizeof(ram));
        bankCount = rom ? (rom->size() / 0x1000) : 3;
        if (bankCount < 1) bankCount = 1;
    }

    uint8_t read(uint16_t addr) const override {
        addr &= 0x1FFF;
        checkHotspot(addr);
        if (addr >= 0x1000) {
            // RAM read: $1100-$11FF
            if (addr >= 0x1100 && addr < 0x1200)
                return ram[addr - 0x1100];
            // RAM write area reads return 0
            if (addr >= 0x1000 && addr < 0x1100)
                return 0;
            int idx = (currentBank % bankCount) * 0x1000 + (addr - 0x1000);
            if (rom && idx < rom->size())
                return static_cast<uint8_t>(rom->at(idx));
        }
        return 0;
    }

    void write(uint16_t addr, uint8_t value) override {
        addr &= 0x1FFF;
        checkHotspot(addr);
        // RAM write: $1000-$10FF
        if (addr >= 0x1000 && addr < 0x1100)
            ram[addr - 0x1000] = value;
    }

    void reset() override {
        currentBank = 2;
        memset(ram, 0, sizeof(ram));
    }

    void serialize(QDataStream &ds) const override {
        ds << static_cast<qint32>(currentBank);
        for (int i = 0; i < 256; i++) ds << ram[i];
    }
    void deserialize(QDataStream &ds) override {
        qint32 cb; ds >> cb; currentBank = cb;
        for (int i = 0; i < 256; i++) ds >> ram[i];
    }

private:
    void checkHotspot(uint16_t addr) const {
        if (addr == 0x1FF8) currentBank = 0;
        else if (addr == 0x1FF9) currentBank = 1;
        else if (addr == 0x1FFA) currentBank = 2;
    }

    mutable int currentBank;
    int bankCount;
    uint8_t ram[256];
};
