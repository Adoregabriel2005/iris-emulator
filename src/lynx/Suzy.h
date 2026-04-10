#pragma once

#include <cstdint>
#include <QImage>
#include <QDataStream>

class LynxCart;

// Atari Lynx Suzy — sprite engine, math coprocessor, joystick, cartridge I/O
// Register space: $FC00–$FCFF
class Suzy {
public:
    Suzy();

    void setRAM(uint8_t *ram) { m_ram = ram; }
    void setCart(LynxCart *cart) { m_cart = cart; }

    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t val);

    // Process sprites (called when SPRGO is written with bit 0 set)
    // Returns number of CPU cycles consumed by sprite engine
    int processSprites();

    bool isSpriteProcessing() const { return m_sprite_processing; }

    // Joystick/button input (directly mapped from host)
    void setJoystick(uint8_t val)  { m_joystick = val; }
    void setSwitches(uint8_t val)  { m_switches = val; }

    // Math results readable after computation
    void serialize(QDataStream &ds) const;
    void deserialize(QDataStream &ds);

private:
    uint8_t *m_ram = nullptr;
    LynxCart *m_cart = nullptr;

    // Sprite engine state
    bool m_sprite_processing = false;
    uint16_t m_scb_next = 0;  // next SCB address
    uint16_t m_vidbas = 0;    // video buffer base in RAM
    uint16_t m_collbas = 0;   // collision buffer base in RAM

    // Sprite control registers
    uint8_t m_sprctl0 = 0;
    uint8_t m_sprctl1 = 0;
    uint8_t m_sprcoll = 0;
    uint8_t m_sprinit = 0;

    // Sprite positioning
    int16_t m_hoff = 0, m_voff = 0;
    int16_t m_tilt = 0;
    int16_t m_stretch = 0;

    // Math coprocessor registers
    // 16x16 -> 32 multiply, 32/16 -> 16+16 divide
    uint8_t m_mathA = 0, m_mathB = 0, m_mathC = 0, m_mathD = 0; // multiplicand/divisor
    uint8_t m_mathE = 0, m_mathF = 0, m_mathG = 0, m_mathH = 0; // result
    uint8_t m_mathJ = 0, m_mathK = 0, m_mathL = 0, m_mathM = 0;
    uint8_t m_mathN = 0, m_mathP = 0;
    bool m_math_sign = false; // signed operation
    bool m_math_accumulate = false;

    // Input
    uint8_t m_joystick = 0;  // $FCB2 — 8 direction bits
    uint8_t m_switches = 0;  // $FCB3 — buttons

    // Internal registers array for general access
    uint8_t m_regs[256] = {};

    // Suzy bus enable
    bool m_suzy_bus_en = false;

    // Sprite line decoder state
    uint16_t m_sprdoff = 0;
    uint16_t m_sprdline = 0;
    uint16_t m_sprhsiz = 0;
    uint16_t m_sprvsiz = 0;
    uint16_t m_hsizacum = 0;
    uint16_t m_vsizacum = 0;
    uint16_t m_hsizoff = 0x007F;
    uint16_t m_vsizoff = 0x007F;
    uint32_t m_line_shift_reg = 0;
    int m_line_shift_reg_count = 0;
    int m_line_repeat_count = 0;
    int m_line_packet_bits_left = 0;
    int m_line_type = 0;
    int m_line_pixel = 0;
    uint16_t m_line_data_ptr = 0;
    uint16_t m_line_base_address = 0;
    uint16_t m_line_collision_address = 0;
    uint8_t m_collision = 0;
    uint8_t m_pen_index[16] = {};

    static constexpr int LINE_ERROR = 0;
    static constexpr int LINE_LITERAL = 1;
    static constexpr int LINE_PACKED = 2;
    static constexpr int LINE_ABS_LITERAL = 3;
    static constexpr int LINE_END = 0xFF;

    uint16_t initSpriteLine(uint16_t addr, bool literalMode, int pixelBits, int voff);
    int getSpriteLineBits(int bits);
    int getSpriteLinePixel(int pixelBits);

    // Sprite rendering helpers
    void renderSprite(uint16_t scb_addr);
    void doMathMultiply();
    void doMathDivide();
    void plotPixel(int x, int y, uint8_t color, uint8_t collisionNum);
};
