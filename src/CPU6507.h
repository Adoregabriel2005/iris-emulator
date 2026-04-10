#pragma once

#include <cstdint>
#include <functional>
#include <QDataStream>

class CPU6507 {
public:
    using ReadFunc = std::function<uint8_t(uint16_t)>;
    using WriteFunc = std::function<void(uint16_t,uint8_t)>;
    using CycleFunc = std::function<void()>;

    CPU6507(ReadFunc r, WriteFunc w);
    ~CPU6507();

    void reset();
    int step(); // execute one instruction, return cycles consumed

    // Called once per CPU cycle during instruction execution (for cycle-accurate TIA sync)
    CycleFunc cycleCallback;

    void serialize(QDataStream &ds) const;
    void deserialize(QDataStream &ds);

    uint8_t A = 0;
    uint8_t X = 0;
    uint8_t Y = 0;
    uint8_t P = 0x24; // processor status
    uint8_t SP = 0xFD;
    uint16_t PC = 0;

private:
    ReadFunc read;
    WriteFunc write;
    static const uint16_t ADDR_MASK = 0x1FFF; // 13-bit address bus for 6507

    int m_busAccess = 0; // counts bus accesses within current instruction

    inline uint8_t rd(uint16_t addr) {
        if (m_busAccess > 0 && cycleCallback) cycleCallback();
        m_busAccess++;
        return read(addr & ADDR_MASK);
    }
    inline void wr(uint16_t addr, uint8_t v) {
        if (m_busAccess > 0 && cycleCallback) cycleCallback();
        m_busAccess++;
        write(addr & ADDR_MASK, v);
    }
    inline uint16_t rd16(uint16_t addr) { uint8_t lo = rd(addr); uint8_t hi = rd((addr + 1) & ADDR_MASK); return uint16_t((hi << 8) | lo); }
    inline void setZN(uint8_t v) { if (v == 0) P |= 0x02; else P &= ~0x02; P = (P & ~0x80) | (v & 0x80); }

    // Fetch helpers / addressing modes
    uint8_t fetch();
    uint16_t zp_addr();
    uint16_t zp_x_addr();
    uint16_t zp_y_addr();
    uint16_t abs_addr();
    uint16_t abs_x_addr(bool &pageCross);
    uint16_t abs_y_addr(bool &pageCross);
    uint16_t ind_addr(); // JMP (ind)
    uint16_t ind_x_addr();
    uint16_t ind_y_addr(bool &pageCross);

    void push(uint8_t v);
    uint8_t pull();
};
