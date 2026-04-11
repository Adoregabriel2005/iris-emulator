#pragma once
#include "Mapper.h"

// Tigervision 3E mapper (extended 3F)
// Up to 512KB ROM in 2KB banks + 32KB RAM in 1KB banks
// $1000-$17FF: switchable ROM or RAM
// $1800-$1FFF: fixed to last 2KB ROM bank
// Write to $003E: select 1KB RAM bank for lower window
// Write to $003F: select 2KB ROM bank for lower window
class Mapper3E : public Mapper {
public:
    explicit Mapper3E(const QByteArray *romData)
        : Mapper(romData), romBank(0), ramBank(0), ramMode(false)
    {
        romBankCount = rom ? (rom->size() / 0x800) : 1;
        if (romBankCount < 1) romBankCount = 1;
        memset(ram, 0, sizeof(ram));
    }

    uint8_t read(uint16_t addr) const override {
        addr &= 0x1FFF;
        if (addr >= 0x1000) {
            int offset = addr - 0x1000;
            if (offset < 0x800) {
                // Lower window: ROM or RAM
                if (ramMode) {
                    int idx = (ramBank % 32) * 0x400 + (offset & 0x3FF);
                    return ram[idx % sizeof(ram)];
                } else {
                    int idx = (romBank % romBankCount) * 0x800 + offset;
                    if (rom && idx < rom->size())
                        return static_cast<uint8_t>(rom->at(idx));
                }
            } else {
                // Upper window: fixed last ROM bank
                int idx = (romBankCount - 1) * 0x800 + (offset - 0x800);
                if (rom && idx < rom->size())
                    return static_cast<uint8_t>(rom->at(idx));
            }
        }
        return 0;
    }

    void write(uint16_t addr, uint8_t value) override {
        uint16_t masked = addr & 0x1FFF;
        if (masked == 0x003E) {
            ramBank = value & 0x1F;
            ramMode = true;
        } else if (masked == 0x003F) {
            romBank = value;
            ramMode = false;
        } else if (ramMode && masked >= 0x1000 && masked < 0x1800) {
            int idx = (ramBank % 32) * 0x400 + ((masked - 0x1000) & 0x3FF);
            ram[idx % sizeof(ram)] = value;
        }
    }

    void reset() override {
        romBank = 0; ramBank = 0; ramMode = false;
        memset(ram, 0, sizeof(ram));
    }

    void serialize(QDataStream &ds) const override {
        ds << static_cast<qint32>(romBank) << static_cast<qint32>(ramBank) << ramMode;
        for (int i = 0; i < (int)sizeof(ram); i++) ds << ram[i];
    }
    void deserialize(QDataStream &ds) override {
        qint32 rb, rab; ds >> rb >> rab >> ramMode;
        romBank = rb; ramBank = rab;
        for (int i = 0; i < (int)sizeof(ram); i++) ds >> ram[i];
    }

private:
    int romBank, romBankCount;
    int ramBank;
    bool ramMode;
    uint8_t ram[32 * 1024];
};
