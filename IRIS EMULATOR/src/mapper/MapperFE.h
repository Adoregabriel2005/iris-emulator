#pragma once
#include "Mapper.h"

// Activision FE mapper (8KB, 2x4KB banks)
// Bank switching triggered by JSR/RTS — detects specific opcode sequences.
// Specifically: after a JSR to $xxFE or $xxFF, the next byte selects the bank.
// Simpler detection: monitor writes to $01FE/$01FF (stack during JSR).
// Implementation: track last byte written to stack page $01xx.
// When $FE or $FF is written to stack, next stack write selects bank.
class MapperFE : public Mapper {
public:
    explicit MapperFE(const QByteArray *romData)
        : Mapper(romData), currentBank(0), lastStackHigh(0), pendingSwitch(false)
    {}

    uint8_t read(uint16_t addr) const override {
        addr &= 0x1FFF;
        if (addr >= 0x1000) {
            int idx = (currentBank * 0x1000) + (addr - 0x1000);
            if (rom && idx < rom->size())
                return static_cast<uint8_t>(rom->at(idx));
        }
        return 0;
    }

    void write(uint16_t addr, uint8_t value) override {
        // FE mapper: monitor stack writes at $01FE and $01FF
        // JSR pushes PC+2 high byte then low byte to stack
        // The high byte of the return address encodes the bank
        if ((addr & 0xFF00) == 0x0100) {
            uint8_t stackAddr = addr & 0xFF;
            if (stackAddr == 0xFE || stackAddr == 0xFF) {
                // High byte of return address: bit 5 selects bank
                currentBank = (value >> 5) & 1;
            }
        }
    }

    void reset() override { currentBank = 0; }

    void serialize(QDataStream &ds) const override {
        ds << static_cast<qint32>(currentBank);
    }
    void deserialize(QDataStream &ds) override {
        qint32 cb; ds >> cb; currentBank = cb;
    }

private:
    mutable int currentBank;
    uint8_t lastStackHigh;
    bool pendingSwitch;
};
