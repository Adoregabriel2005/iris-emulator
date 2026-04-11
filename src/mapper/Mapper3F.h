#pragma once
#include "Mapper.h"

// Tigervision 3F mapper
// ROM up to 512KB in 2KB banks.
// $1800-$1FFF: fixed to last 2KB bank
// $1000-$17FF: switchable — write to $003F selects bank
// Write to any address $0000-$003F sets the bank for the lower window.
class Mapper3F : public Mapper {
public:
    explicit Mapper3F(const QByteArray *romData)
        : Mapper(romData)
    {
        bankCount = rom ? (rom->size() / 0x800) : 1;
        if (bankCount < 1) bankCount = 1;
        currentBank = 0;
    }

    uint8_t read(uint16_t addr) const override {
        addr &= 0x1FFF;
        if (addr >= 0x1000) {
            int offset = addr - 0x1000;
            int bank, bankOffset;
            if (offset < 0x800) {
                // $1000-$17FF: switchable
                bank = currentBank % bankCount;
                bankOffset = offset;
            } else {
                // $1800-$1FFF: fixed to last bank
                bank = bankCount - 1;
                bankOffset = offset - 0x800;
            }
            int idx = bank * 0x800 + bankOffset;
            if (rom && idx < rom->size())
                return static_cast<uint8_t>(rom->at(idx));
        }
        return 0;
    }

    void write(uint16_t addr, uint8_t value) override {
        // Write to $0000-$003F switches bank
        if ((addr & 0x1FFF) <= 0x003F)
            currentBank = value % bankCount;
    }

    void reset() override { currentBank = 0; }

    void serialize(QDataStream &ds) const override {
        ds << static_cast<qint32>(currentBank);
        ds << static_cast<qint32>(bankCount);
    }
    void deserialize(QDataStream &ds) override {
        qint32 cb, bc; ds >> cb >> bc;
        currentBank = cb; bankCount = bc;
    }

private:
    mutable int currentBank;
    int bankCount;
};
