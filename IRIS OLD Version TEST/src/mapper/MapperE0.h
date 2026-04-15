#pragma once
#include "Mapper.h"

// Parker Brothers E0 mapper (8KB)
// 4 x 1KB slices:
//   $1000-$13FF: switchable (hotspots $1FE0-$1FE7 select bank 0-7)
//   $1400-$17FF: switchable (hotspots $1FE8-$1FEF select bank 0-7)
//   $1800-$1BFF: switchable (hotspots $1FF0-$1FF7 select bank 0-7)
//   $1C00-$1FFF: fixed to last 1KB bank (bank 7)
class MapperE0 : public Mapper {
public:
    explicit MapperE0(const QByteArray *romData)
        : Mapper(romData)
    {
        sliceBank[0] = 4;
        sliceBank[1] = 5;
        sliceBank[2] = 6;
    }

    uint8_t read(uint16_t addr) const override {
        addr &= 0x1FFF;
        checkHotspot(addr);
        if (addr >= 0x1000) {
            int offset = addr - 0x1000;
            int slice = offset >> 10; // 0-3
            int bank;
            if (slice < 3) {
                bank = sliceBank[slice];
            } else {
                bank = 7; // slice 3 is fixed to last bank
            }
            int romOffset = bank * 0x400 + (offset & 0x3FF);
            if (rom && romOffset < rom->size())
                return static_cast<uint8_t>(rom->at(romOffset));
            return 0;
        }
        return 0;
    }

    void write(uint16_t addr, uint8_t /*value*/) override {
        addr &= 0x1FFF;
        checkHotspot(addr);
    }

    void reset() override {
        sliceBank[0] = 4;
        sliceBank[1] = 5;
        sliceBank[2] = 6;
    }

    void serialize(QDataStream &ds) const override {
        for (int i = 0; i < 3; i++) ds << static_cast<qint32>(sliceBank[i]);
    }

    void deserialize(QDataStream &ds) override {
        for (int i = 0; i < 3; i++) {
            qint32 v; ds >> v; sliceBank[i] = v;
        }
    }

private:
    void checkHotspot(uint16_t addr) const {
        if (addr >= 0x1FE0 && addr <= 0x1FF7) {
            int idx = addr - 0x1FE0;
            int slice = idx / 8;   // 0, 1, or 2
            int bank = idx % 8;    // 0-7
            if (slice < 3) {
                sliceBank[slice] = bank;
            }
        }
    }

    mutable int sliceBank[3]; // bank selected for slices 0-2
};
