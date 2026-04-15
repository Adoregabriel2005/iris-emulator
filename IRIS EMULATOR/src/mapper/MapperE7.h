#pragma once
#include "Mapper.h"
#include <cstring>

// M-Network E7 mapper (16KB ROM + 2KB RAM)
// $1000-$17FF: switchable 2KB bank (hotspots $1FE0-$1FE6 select banks 0-6)
// $1800-$19FF: switchable 256B RAM bank (hotspots $1FE8-$1FEB select 0-3)
//              write $1800-$18FF, read $1900-$19FF
// $1A00-$1FFF: fixed to last 2KB bank (bank 7)
// Hotspot $1FE7: enables RAM in lower window
class MapperE7 : public Mapper {
public:
    explicit MapperE7(const QByteArray *romData)
        : Mapper(romData), romBank(0), ramBank(0), ramEnabled(false)
    {
        memset(ram, 0, sizeof(ram));
        memset(ram256, 0, sizeof(ram256));
    }

    uint8_t read(uint16_t addr) const override {
        addr &= 0x1FFF;
        checkHotspot(addr);
        if (addr < 0x1000) return 0;

        int offset = addr - 0x1000;

        // $1000-$17FF: switchable or RAM
        if (offset < 0x800) {
            if (ramEnabled) {
                // 1KB RAM: write $1000-$13FF, read $1400-$17FF
                if (offset >= 0x400)
                    return ram[offset - 0x400];
                return 0; // write-only area
            }
            int idx = romBank * 0x800 + offset;
            if (rom && idx < rom->size())
                return static_cast<uint8_t>(rom->at(idx));
            return 0;
        }

        // $1800-$19FF: 256B RAM banks
        if (offset >= 0x800 && offset < 0xA00) {
            int ramOffset = offset - 0x800;
            if (ramOffset >= 0x100) // read area $1900-$19FF
                return ram256[ramBank * 256 + (ramOffset - 0x100)];
            return 0; // write-only $1800-$18FF
        }

        // $1A00-$1FFF: fixed last bank (bank 7)
        {
            int idx = 7 * 0x800 + (offset - 0xA00) + 0x200;
            // Last 1.5KB of bank 7
            int fixedIdx = 7 * 0x800 + (addr - 0x1A00);
            if (rom && fixedIdx < rom->size())
                return static_cast<uint8_t>(rom->at(fixedIdx));
        }
        return 0;
    }

    void write(uint16_t addr, uint8_t value) override {
        addr &= 0x1FFF;
        checkHotspot(addr);
        if (addr < 0x1000) return;
        int offset = addr - 0x1000;

        // RAM write areas
        if (ramEnabled && offset < 0x400)
            ram[offset] = value;
        if (offset >= 0x800 && offset < 0x900)
            ram256[ramBank * 256 + (offset - 0x800)] = value;
    }

    void reset() override {
        romBank = 0; ramBank = 0; ramEnabled = false;
        memset(ram, 0, sizeof(ram));
        memset(ram256, 0, sizeof(ram256));
    }

    void serialize(QDataStream &ds) const override {
        ds << static_cast<qint32>(romBank) << static_cast<qint32>(ramBank) << ramEnabled;
        for (int i = 0; i < 1024; i++) ds << ram[i];
        for (int i = 0; i < 1024; i++) ds << ram256[i];
    }
    void deserialize(QDataStream &ds) override {
        qint32 rb, rab; ds >> rb >> rab >> ramEnabled;
        romBank = rb; ramBank = rab;
        for (int i = 0; i < 1024; i++) ds >> ram[i];
        for (int i = 0; i < 1024; i++) ds >> ram256[i];
    }

private:
    void checkHotspot(uint16_t addr) const {
        if (addr >= 0x1FE0 && addr <= 0x1FE6) {
            romBank = addr - 0x1FE0;
            ramEnabled = false;
        } else if (addr == 0x1FE7) {
            ramEnabled = true;
        } else if (addr >= 0x1FE8 && addr <= 0x1FEB) {
            ramBank = addr - 0x1FE8;
        }
    }

    mutable int romBank;
    mutable int ramBank;
    mutable bool ramEnabled;
    uint8_t ram[1024];     // 1KB switchable RAM
    uint8_t ram256[1024];  // 4x256B RAM banks
};
