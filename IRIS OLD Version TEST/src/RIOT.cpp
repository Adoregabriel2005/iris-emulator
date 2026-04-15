#include "RIOT.h"
#include <cstring>

RIOT::RIOT()
{
    memset(ram, 0, sizeof(ram));
    SWACNT = 0;
    SWBCNT = 0;
    portAOut = 0;
    timerValue = 0;
    timerInterval = 1024;
    subCounter = 1024;
    timerUnderflowMode = false;
    timerInterruptFlag = false;
}

RIOT::~RIOT() {}

// ════════════════════════════════════════════════════════════════════════
// Timer tick — called once per CPU cycle
// ════════════════════════════════════════════════════════════════════════
void RIOT::tick(int cpuCycles)
{
    for (int i = 0; i < cpuCycles; ++i) {
        if (!timerUnderflowMode) {
            // Normal countdown mode: decrement sub-counter
            subCounter--;
            if (subCounter <= 0) {
                subCounter = timerInterval;
                timerValue--;
                if (timerValue < 0) {
                    timerUnderflowMode = true;
                    timerInterruptFlag = true;
                    timerValue = 0xFF; // wrap and count down at 1-cycle rate
                }
            }
        } else {
            // After underflow: count down every cycle
            timerValue--;
            if (timerValue < 0) {
                timerValue = 0xFF; // keep wrapping
            }
        }
    }
}

// ════════════════════════════════════════════════════════════════════════
// Register read
// addr is the full 13-bit bus address. RIOT is selected when A12=0, A7=1, A9=1.
// Decoding: A2 selects I/O (0) vs Timer (1). A0-A1 select sub-register.
// ════════════════════════════════════════════════════════════════════════
uint8_t RIOT::read(uint16_t addr) const
{
    // Use bits A0-A2 for register selection
    if (addr & 0x04) {
        // Timer area (A2=1)
        if (addr & 0x01) {
            // TIMINT (A0=1) — timer interrupt status
            return timerInterruptFlag ? 0x80 : 0x00;
        } else {
            // INTIM (A0=0) — timer value; clears interrupt flag (NOT underflow mode)
            timerInterruptFlag = false;
            return static_cast<uint8_t>(timerValue & 0xFF);
        }
    }

    // I/O registers (A2=0)
    switch (addr & 0x03) {
    case 0x00: { // SWCHA — Joystick port A
        // P0: D7=right, D6=left, D5=down, D4=up (active low)
        // P1: D3=right, D2=left, D1=down, D0=up
        uint8_t input = 0xFF;
        if (js.right) input &= ~0x80;
        if (js.left)  input &= ~0x40;
        if (js.down)  input &= ~0x20;
        if (js.up)    input &= ~0x10;
        if (js.p1right) input &= ~0x08;
        if (js.p1left)  input &= ~0x04;
        if (js.p1down)  input &= ~0x02;
        if (js.p1up)    input &= ~0x01;
        return (input & ~SWACNT) | (portAOut & SWACNT);
    }
    case 0x01: // SWACNT
        return SWACNT;
    case 0x02: { // SWCHB — Console switches
        uint8_t v = 0xFF;
        if (resetPressed)   v &= ~0x01; // bit 0 = Reset
        if (selectPressed)  v &= ~0x02; // bit 1 = Select
        if (!colorMode)     v &= ~0x08; // bit 3 = Color/BW
        if (!p0diff)        v &= ~0x40; // bit 6 = P0 difficulty
        if (!p1diff)        v &= ~0x80; // bit 7 = P1 difficulty
        return v;
    }
    case 0x03: // SWBCNT
        return SWBCNT;
    }
    return 0;
}

// ════════════════════════════════════════════════════════════════════════
// Register write
// Decoding: A4 selects I/O (0) vs Timer (1) for writes.
// Timer writes: A0-A1 select interval (TIM1T/TIM8T/TIM64T/T1024T).
// I/O writes: A0-A1 select register (SWCHA/SWACNT/SWCHB/SWBCNT).
// ════════════════════════════════════════════════════════════════════════
void RIOT::write(uint16_t addr, uint8_t val)
{
    if (addr & 0x10) {
        // Timer write (A4=1) — resets to interval counting mode
        timerUnderflowMode = false;
        timerInterruptFlag = false;
        timerValue = val;
        switch (addr & 0x03) {
        case 0x00: timerInterval = 1;    subCounter = 1;    break; // TIM1T
        case 0x01: timerInterval = 8;    subCounter = 8;    break; // TIM8T
        case 0x02: timerInterval = 64;   subCounter = 64;   break; // TIM64T
        case 0x03: timerInterval = 1024; subCounter = 1024; break; // T1024T
        }
        return;
    }

    // I/O write (A4=0)
    switch (addr & 0x03) {
    case 0x00: portAOut = val; return;  // SWCHA output latch
    case 0x01: SWACNT = val;  return;  // SWACNT DDR
    case 0x02: return;                 // SWCHB (read-only for console)
    case 0x03: SWBCNT = val;  return;  // SWBCNT DDR
    }
}

// ════════════════════════════════════════════════════════════════════════
// Serialization
// ════════════════════════════════════════════════════════════════════════
void RIOT::serialize(QDataStream &ds) const
{
    for (int i = 0; i < 128; ++i) ds << ram[i];
    ds << SWACNT << SWBCNT << portAOut;
    ds << static_cast<qint32>(timerValue);
    ds << static_cast<qint32>(timerInterval);
    ds << static_cast<qint32>(subCounter);
    ds << timerUnderflowMode;
    ds << timerInterruptFlag;
}

void RIOT::deserialize(QDataStream &ds)
{
    for (int i = 0; i < 128; ++i) ds >> ram[i];
    ds >> SWACNT >> SWBCNT >> portAOut;
    qint32 tv = 0, ti = 0, sc = 0;
    ds >> tv >> ti >> sc;
    timerValue = tv;
    timerInterval = ti;
    subCounter = sc;
    ds >> timerUnderflowMode;
    ds >> timerInterruptFlag;
}
