#pragma once

#include <cstdint>
#include "SDLInput.h"
#include <QDataStream>

// 6532 RIOT — RAM + I/O + Timer
// Addresses in the Atari 2600 memory map: 0x0280-0x02FF
// Register offsets (addr & 0x7F):
//   0x00 SWCHA   — Port A data (joystick directions, active low)
//   0x01 SWACNT  — Port A DDR
//   0x02 SWCHB   — Port B data (console switches)
//   0x03 SWBCNT  — Port B DDR
//   0x04 INTIM   — Timer output (read)
//   0x05 TIMINT  — Timer interrupt status (read)
//   Write (addr & 0x1F):
//   0x14 TIM1T   — Set 1-cycle timer
//   0x15 TIM8T   — Set 8-cycle timer
//   0x16 TIM64T  — Set 64-cycle timer
//   0x17 T1024T  — Set 1024-cycle timer

class RIOT {
public:
    RIOT();
    ~RIOT();

    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t val);

    // Called once per CPU cycle to decrement timer
    void tick(int cpuCycles);

    void setJoystickState(const JoystickState &s) { js = s; }

    // Console switches
    void setSelect(bool s) { selectPressed = s; }
    void setReset(bool r)  { resetPressed = r; }
    void setColorBW(bool color) { colorMode = color; }
    void setP0Diff(bool a) { p0diff = a; }
    void setP1Diff(bool a) { p1diff = a; }

    bool getP0Diff() const { return p0diff; }
    bool getColorMode() const { return colorMode; }

    void serialize(QDataStream &ds) const;
    void deserialize(QDataStream &ds);

private:
    uint8_t ram[128];
    JoystickState js;

    // I/O
    uint8_t SWACNT; // port A DDR
    uint8_t SWBCNT; // port B DDR
    uint8_t portAOut; // output latch for SWCHA

    // Console switch state
    bool selectPressed = false;
    bool resetPressed  = false;
    bool colorMode     = true;  // true = color, false = B&W
    bool p0diff        = true;  // true = A (pro), false = B (amateur)
    bool p1diff        = true;

    // Timer
    int32_t timerValue;   // current timer counter (can go negative after underflow)
    int32_t timerInterval; // divisor (1, 8, 64, 1024)
    int32_t subCounter;   // sub-cycle counter for the interval
    bool timerUnderflowMode = false;  // true = counting every cycle after underflow
    mutable bool timerInterruptFlag = false;  // mutable: cleared on INTIM read, set on underflow
};
