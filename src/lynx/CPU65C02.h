#pragma once

#include <cstdint>
#include <functional>
#include <QDataStream>

// WDC 65C02 CPU — full 16-bit address space, extra opcodes vs vanilla 6502
// Used by the Atari Lynx (4 MHz clock)
class CPU65C02 {
public:
    using ReadFunc  = std::function<uint8_t(uint16_t)>;
    using WriteFunc = std::function<void(uint16_t, uint8_t)>;

    CPU65C02(ReadFunc r, WriteFunc w);
    ~CPU65C02();

    void reset();
    int  step(); // execute one instruction, return cycles consumed

    void triggerNMI();
    void triggerIRQ();

    void serialize(QDataStream &ds) const;
    void deserialize(QDataStream &ds);

    // Registers (public for debugger access)
    uint8_t  A  = 0;
    uint8_t  X  = 0;
    uint8_t  Y  = 0;
    uint8_t  P  = 0x24; // NV-BDIZC
    uint8_t  SP = 0xFD;
    uint16_t PC = 0;
    bool     halted = false; // WAI instruction

private:
    ReadFunc  read;
    WriteFunc write;

    bool m_nmi_pending = false;
    bool m_irq_pending = false;
    bool m_nmi_edge    = false; // NMI is edge-triggered

    inline uint8_t rd(uint16_t addr)            { return read(addr); }
    inline void    wr(uint16_t addr, uint8_t v) { write(addr, v); }
    inline uint16_t rd16(uint16_t addr) {
        uint8_t lo = rd(addr);
        uint8_t hi = rd(addr + 1);
        return (uint16_t(hi) << 8) | lo;
    }

    // Status flag helpers
    inline void setZN(uint8_t v) {
        P &= ~0x82;
        if (v == 0) P |= 0x02;
        P |= (v & 0x80);
    }
    inline bool getC() const { return P & 0x01; }
    inline bool getZ() const { return P & 0x02; }
    inline bool getI() const { return P & 0x04; }
    inline bool getD() const { return P & 0x08; }
    inline bool getV() const { return P & 0x40; }
    inline bool getN() const { return P & 0x80; }
    inline void setC(bool v) { if (v) P |= 0x01; else P &= ~0x01; }
    inline void setV(bool v) { if (v) P |= 0x40; else P &= ~0x40; }

    // Addressing modes
    uint8_t  fetch();
    uint16_t addr_zp();
    uint16_t addr_zp_x();
    uint16_t addr_zp_y();
    uint16_t addr_abs();
    uint16_t addr_abs_x(bool &pageCross);
    uint16_t addr_abs_y(bool &pageCross);
    uint16_t addr_ind_x();      // (zp,X)
    uint16_t addr_ind_y(bool &pageCross); // (zp),Y
    uint16_t addr_zp_ind();     // 65C02: (zp) — zero page indirect
    uint16_t addr_abs_ind_x();  // 65C02: (abs,X) — for JMP

    // Stack ops
    void push8(uint8_t v);
    void push16(uint16_t v);
    uint8_t  pull8();
    uint16_t pull16();

    // ALU operations
    void op_adc(uint8_t val);
    void op_sbc(uint8_t val);
    void op_cmp(uint8_t reg, uint8_t val);

    // Branch helper
    int doBranch(bool cond);
};
