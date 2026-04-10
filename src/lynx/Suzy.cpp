#include "Suzy.h"
#include "LynxCart.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include <QDebug>

Suzy::Suzy()
{
    std::memset(m_regs, 0, sizeof(m_regs));

    m_ram = nullptr;
    m_cart = nullptr;
    m_sprite_processing = false;
    m_scb_next = 0;
    m_vidbas = 0;
    m_collbas = 0;
    m_sprctl0 = 0;
    m_sprctl1 = 0;
    m_sprcoll = 0;
    m_sprinit = 0;
    m_hoff = 0;
    m_voff = 0;
    m_tilt = 0;
    m_stretch = 0;
    m_mathA = m_mathB = m_mathC = m_mathD = 0;
    m_mathE = m_mathF = m_mathG = m_mathH = 0;
    m_mathJ = m_mathK = m_mathL = m_mathM = 0;
    m_mathN = 0;
    m_mathP = 0;
    m_math_sign = false;
    m_math_accumulate = false;
    m_joystick = 0;
    m_switches = 0;
    m_suzy_bus_en = false;
    m_sprdoff = 0;
    m_sprdline = 0;
    m_sprhsiz = 0;
    m_sprvsiz = 0;
    m_hsizacum = 0;
    m_vsizacum = 0;
    m_hsizoff = 0x007F;
    m_vsizoff = 0x007F;
    m_line_shift_reg = 0;
    m_line_shift_reg_count = 0;
    m_line_repeat_count = 0;
    m_line_packet_bits_left = 0;
    m_line_type = 0;
    m_line_pixel = 0;
    m_line_data_ptr = 0;
    m_line_base_address = 0;
    m_line_collision_address = 0;
    m_collision = 0;
    std::fill(std::begin(m_pen_index), std::end(m_pen_index), 0);
}

// ── Register read ($FC00–$FCFF) — Correct Lynx register map ──

uint8_t Suzy::read(uint16_t addr)
{
    uint8_t reg = addr & 0xFF;

    switch (reg) {
    // Sprite engine registers (normally write-only, but some readable)
    case 0x00: return m_regs[0x00]; // TMPADRL
    case 0x01: return m_regs[0x01]; // TMPADRH
    case 0x04: return static_cast<uint8_t>(m_hoff); // HOFFL
    case 0x05: return static_cast<uint8_t>(m_hoff >> 8); // HOFFH
    case 0x06: return static_cast<uint8_t>(m_voff); // VOFFL
    case 0x07: return static_cast<uint8_t>(m_voff >> 8); // VOFFH
    case 0x08: return static_cast<uint8_t>(m_vidbas);  // VIDBASL
    case 0x09: return static_cast<uint8_t>(m_vidbas >> 8); // VIDBASH
    case 0x0A: return static_cast<uint8_t>(m_collbas); // COLLBASL
    case 0x0B: return static_cast<uint8_t>(m_collbas >> 8); // COLLBASH
    case 0x10: return static_cast<uint8_t>(m_scb_next); // SCBNEXTL
    case 0x11: return static_cast<uint8_t>(m_scb_next >> 8); // SCBNEXTH

    // Math result registers
    case 0x60: return m_mathH;  // MATHH
    case 0x61: return m_mathG;  // MATHG
    case 0x62: return m_mathF;  // MATHF
    case 0x63: return m_mathE;  // MATHE
    case 0x6C: return m_mathM;  // MATHM
    case 0x6D: return m_mathL;  // MATHL
    case 0x6E: return m_mathK;  // MATHK
    case 0x6F: return m_mathJ;  // MATHJ
    case 0x52: return m_mathD;  // MATHD
    case 0x53: return m_mathC;  // MATHC
    case 0x54: return m_mathB;  // MATHB
    case 0x55: return m_mathA;  // MATHA
    case 0x56: return m_mathP;  // MATHP
    case 0x57: return m_mathN;  // MATHN

    // Hardware revision
    case 0x88: return 0x01; // SUZYHREV
    case 0x89: return 0x01; // SUZYSREV

    // Sprite system status
    case 0x92: { // SPRSYS read
        uint8_t status = 0;
        if (m_sprite_processing) status |= 0x01; // sprite engine busy
        if (m_math_sign)         status |= 0x80; // math sign
        return status;
    }

    // Joystick & Switches
    case 0xB0: return m_joystick; // JOYSTICK
    case 0xB1: return m_switches; // SWITCHES

    // Cart read — RCART0 / RCART1
    case 0xB2: return m_cart ? m_cart->peek0() : 0xFF; // RCART0
    case 0xB3: return m_cart ? m_cart->peek1() : 0xFF; // RCART1

    default:
        return m_regs[reg];
    }
}

// ── Register write ($FC00–$FCFF) — Correct Lynx register map ──

void Suzy::write(uint16_t addr, uint8_t val)
{
    uint8_t reg = addr & 0xFF;
    m_regs[reg] = val;

    switch (reg) {
    // Sprite positioning — HOFF/VOFF
    case 0x04: m_hoff = (m_hoff & 0xFF00) | val; break; // HOFFL
    case 0x05: m_hoff = (m_hoff & 0x00FF) | (int16_t(val) << 8); break; // HOFFH
    case 0x06: m_voff = (m_voff & 0xFF00) | val; break; // VOFFL
    case 0x07: m_voff = (m_voff & 0x00FF) | (int16_t(val) << 8); break; // VOFFH

    // Video buffer base — VIDBAS at $FC08-$FC09
    case 0x08: m_vidbas = (m_vidbas & 0xFF00) | val; break;
    case 0x09: m_vidbas = (m_vidbas & 0x00FF) | (uint16_t(val) << 8); break;

    // Collision buffer base — COLLBAS at $FC0A-$FC0B
    case 0x0A: m_collbas = (m_collbas & 0xFF00) | val; break;
    case 0x0B: m_collbas = (m_collbas & 0x00FF) | (uint16_t(val) << 8); break;

    // SCB next address — $FC10-$FC11
    case 0x10: m_scb_next = (m_scb_next & 0xFF00) | val; break;
    case 0x11: m_scb_next = (m_scb_next & 0x00FF) | (uint16_t(val) << 8); break;

    // Sprite control — SPRCTL0/1/COLL at $FC80-$FC82
    case 0x80: m_sprctl0 = val; break; // SPRCTL0
    case 0x81: m_sprctl1 = val; break; // SPRCTL1
    case 0x82: m_sprcoll = val; break; // SPRCOLL
    case 0x83: m_sprinit = val; break; // SPRINIT

    // SUZYBUSEN at $FC90
    case 0x90: m_suzy_bus_en = (val & 0x01) != 0; break;

    // SPRGO at $FC91
    case 0x91:
        if (val & 0x01) m_sprite_processing = true;
        break;

    // SPRSYS at $FC92
    case 0x92:
        m_math_sign = (val & 0x80) != 0;
        m_math_accumulate = (val & 0x40) != 0;
        break;

    // Math input registers
    case 0x52: m_mathD = val; break; // MATHD
    case 0x53: m_mathC = val; break; // MATHC
    case 0x54: m_mathB = val; break; // MATHB
    case 0x55: // MATHA — triggers multiply
        m_mathA = val;
        doMathMultiply();
        break;
    case 0x56: m_mathP = val; break; // MATHP
    case 0x57: m_mathN = val; break; // MATHN
    case 0x60: m_mathH = val; break; // MATHH
    case 0x61: m_mathG = val; break; // MATHG
    case 0x62: m_mathF = val; break; // MATHF
    case 0x63: // MATHE — triggers divide
        m_mathE = val;
        doMathDivide();
        break;
    case 0x6C: m_mathM = val; break; // MATHM
    case 0x6D: m_mathL = val; break; // MATHL
    case 0x6E: m_mathK = val; break; // MATHK
    case 0x6F: m_mathJ = val; break; // MATHJ

    // Cart write ports — RCART0/RCART1
    case 0xB2: if (m_cart) m_cart->poke0(val); break; // RCART0
    case 0xB3: if (m_cart) m_cart->poke1(val); break; // RCART1

    default:
        break;
    }
}

// ── Sprite processing ──

int Suzy::processSprites()
{
    if (!m_sprite_processing || !m_ram) {
        m_sprite_processing = false;
        return 0;
    }

    int totalCycles = 0;
    uint16_t scb = m_scb_next;
    int maxSprites = 512; // safety limit
    int spriteCount = 0;

    while (scb != 0 && maxSprites-- > 0) {
        renderSprite(scb);
        totalCycles += 80; // approximate cycles per sprite
        spriteCount++;

        // Read next SCB address from current SCB
        uint16_t next = m_ram[scb] | (uint16_t(m_ram[scb + 1]) << 8);
        if (next == scb) break; // self-referencing = end
        scb = next;
    }

    static int s_sprite_log = 0;
    if (s_sprite_log < 10 && spriteCount > 0) {
        s_sprite_log++;
        qDebug() << "processSprites: count=" << spriteCount
                 << "vidbas=$" << Qt::hex << m_vidbas
                 << "first_scb=$" << Qt::hex << m_scb_next;
    }

    m_sprite_processing = false;
    return totalCycles;
}

void Suzy::renderSprite(uint16_t scb_addr)
{
    if (!m_ram) return;

    uint8_t sprctl0 = m_ram[(scb_addr + 2) & 0xFFFF];
    uint8_t sprctl1 = m_ram[(scb_addr + 3) & 0xFFFF];
    uint8_t sprcoll = m_ram[(scb_addr + 4) & 0xFFFF];

    uint16_t dataAddr = m_ram[(scb_addr + 5) & 0xFFFF] |
                        (uint16_t(m_ram[(scb_addr + 6) & 0xFFFF]) << 8);

    int16_t hpos = (int16_t)(m_ram[(scb_addr + 7) & 0xFFFF] |
                   (uint16_t(m_ram[(scb_addr + 8) & 0xFFFF]) << 8));
    int16_t vpos = (int16_t)(m_ram[(scb_addr + 9) & 0xFFFF] |
                   (uint16_t(m_ram[(scb_addr + 10) & 0xFFFF]) << 8));
    uint16_t hsize = m_ram[(scb_addr + 11) & 0xFFFF] | (uint16_t(m_ram[(scb_addr + 12) & 0xFFFF]) << 8);
    uint16_t vsize = m_ram[(scb_addr + 13) & 0xFFFF] | (uint16_t(m_ram[(scb_addr + 14) & 0xFFFF]) << 8);
    if (hsize == 0) hsize = 256;
    if (vsize == 0) vsize = 256;

    int pixelBits = ((sprctl0 >> 6) & 0x03) + 1;
    bool hFlip = (sprctl0 & 0x20) != 0;
    bool vFlip = (sprctl0 & 0x10) != 0;
    bool startLeft = (sprctl1 & 0x01) != 0;
    bool startUp = (sprctl1 & 0x02) != 0;
    bool skipSprite = (sprctl1 & 0x04) != 0;
    bool reloadPalette = (sprctl1 & 0x08) != 0;
    int reloadDepth = (sprctl1 >> 4) & 0x03;
    bool literalMode = (sprctl1 & 0x80) != 0;

    if (skipSprite) {
        return;
    }

    uint16_t scb_data_ptr = (scb_addr + 15) & 0xFFFF;
    m_sprdline = dataAddr;
    m_collision = 0;

    if (reloadDepth >= 1) {
        m_sprhsiz = m_ram[scb_data_ptr & 0xFFFF] | (uint16_t(m_ram[(scb_data_ptr + 1) & 0xFFFF]) << 8);
        scb_data_ptr = (scb_data_ptr + 2) & 0xFFFF;
        m_sprvsiz = m_ram[scb_data_ptr & 0xFFFF] | (uint16_t(m_ram[(scb_data_ptr + 1) & 0xFFFF]) << 8);
        scb_data_ptr = (scb_data_ptr + 2) & 0xFFFF;
    }

    if (reloadDepth >= 2) {
        m_stretch = m_ram[scb_data_ptr & 0xFFFF] | (uint16_t(m_ram[(scb_data_ptr + 1) & 0xFFFF]) << 8);
        scb_data_ptr = (scb_data_ptr + 2) & 0xFFFF;
    }

    if (reloadDepth >= 3) {
        m_tilt = m_ram[scb_data_ptr & 0xFFFF] | (uint16_t(m_ram[(scb_data_ptr + 1) & 0xFFFF]) << 8);
        scb_data_ptr = (scb_data_ptr + 2) & 0xFFFF;
    }

    if (!reloadPalette) {
        for (int idx = 0; idx < 8; idx++) {
            uint8_t palette = m_ram[scb_data_ptr & 0xFFFF];
            m_pen_index[idx * 2] = (palette >> 4) & 0x0F;
            m_pen_index[idx * 2 + 1] = palette & 0x0F;
            scb_data_ptr = (scb_data_ptr + 1) & 0xFFFF;
        }
    }

    bool enable_sizing = false;
    bool enable_stretch = false;
    bool enable_tilt = false;
    int16_t tilt_accum = 0;

    if (reloadDepth >= 1) {
        enable_sizing = true;
    }
    if (reloadDepth >= 2) {
        enable_sizing = true;
        enable_stretch = true;
    }
    if (reloadDepth >= 3) {
        enable_sizing = true;
        enable_stretch = true;
        enable_tilt = true;
    }

    if (!enable_sizing) {
        m_sprhsiz = hsize;
        m_sprvsiz = vsize;
    }

    int screen_h_start = m_hoff;
    int screen_h_end = screen_h_start + 160;
    int screen_v_start = m_voff;
    int screen_v_end = screen_v_start + 102;
    int world_h_mid = screen_h_start + 80;
    int world_v_mid = screen_v_start + 51;

    bool superclip = (hpos < screen_h_start || hpos >= screen_h_end ||
                      vpos < screen_v_start || vpos >= screen_v_end);

    int quadrant = 0;
    if (startLeft) {
        quadrant = startUp ? 2 : 3;
    } else {
        quadrant = startUp ? 1 : 0;
    }

    int vquadoff = 0;
    int hquadoff = 0;
    for (int loop = 0; loop < 4; loop++) {
        int sprite_h = hpos;
        int sprite_v = vpos;
        bool render = true;

        int hsign = (quadrant == 0 || quadrant == 1) ? 1 : -1;
        int vsign = (quadrant == 0 || quadrant == 3) ? 1 : -1;
        if (vFlip) vsign = -vsign;
        if (hFlip) hsign = -hsign;

        if (superclip) {
            int modquad = quadrant;
            static const int vquadflip[4] = {1, 0, 3, 2};
            static const int hquadflip[4] = {3, 2, 1, 0};
            if (vFlip) modquad = vquadflip[modquad];
            if (hFlip) modquad = hquadflip[modquad];
            switch (modquad) {
            case 3:
                render = ((sprite_h >= screen_h_start || sprite_h <= world_h_mid) &&
                          (sprite_v < screen_v_end || sprite_v >= world_v_mid));
                break;
            case 2:
                render = ((sprite_h >= screen_h_start || sprite_h <= world_h_mid) &&
                          (sprite_v >= screen_v_start || sprite_v <= world_v_mid));
                break;
            case 1:
                render = ((sprite_h < screen_h_end || sprite_h >= world_h_mid) &&
                          (sprite_v >= screen_v_start || sprite_v <= world_v_mid));
                break;
            default:
                render = ((sprite_h < screen_h_end || sprite_h >= world_h_mid) &&
                          (sprite_v < screen_v_end || sprite_v >= world_v_mid));
                break;
            }
        }

        if (render) {
            int voff_line = vpos - screen_v_start;
            m_vsizacum = (vsign == 1) ? m_vsizoff : 0;
            tilt_accum = 0;
            if (loop == 0) vquadoff = vsign;
            if (vsign != vquadoff) voff_line += vsign;

            bool doneSprite = false;
            while (!doneSprite) {
                m_vsizacum += m_sprvsiz;
                int pixel_height = m_vsizacum >> 8;
                m_vsizacum &= 0x00FF;

                uint16_t originalLinePtr = m_sprdline;
                uint16_t nextOffset = initSpriteLine(originalLinePtr, literalMode, pixelBits, 0);
                if (nextOffset == 1) {
                    m_sprdline = (m_sprdline + nextOffset) & 0xFFFF;
                    break;
                }
                if (nextOffset == 0) {
                    loop = 4;
                    break;
                }

                for (int vloop = 0; vloop < pixel_height; vloop++) {
                    if ((vsign == 1 && voff_line >= 102) || (vsign == -1 && voff_line < 0)) {
                        break;
                    }

                    if (voff_line >= 0 && voff_line < 102) {
                        int hstart = hpos - screen_h_start;
                        if (loop == 0) hquadoff = hsign;
                        if (hsign != hquadoff) hstart += hsign;

                        m_hsizacum = m_hsizoff;
                        m_line_base_address = m_vidbas + (voff_line * 80);
                        m_line_collision_address = m_collbas + (voff_line * 80);
                        initSpriteLine(originalLinePtr, literalMode, pixelBits, voff_line);

                        int pixel = getSpriteLinePixel(pixelBits);
                        bool onscreen = false;
                        while (pixel != LINE_END) {
                            m_hsizacum += m_sprhsiz;
                            int pixel_width = m_hsizacum >> 8;
                            m_hsizacum &= 0x00FF;
                            for (int hloop = 0; hloop < pixel_width; hloop++) {
                                if (hstart >= 0 && hstart < 160) {
                                    if (pixel != 0) {
                                        plotPixel(hstart, voff_line, pixel, sprcoll & 0x0F);
                                    }
                                    onscreen = true;
                                } else if (onscreen) {
                                    break;
                                }
                                hstart += hsign;
                            }
                            pixel = getSpriteLinePixel(pixelBits);
                        }
                    }

                    voff_line += vsign;
                    if (enable_stretch) {
                        m_sprhsiz = uint16_t(m_sprhsiz + m_stretch);
                    }
                    if (enable_tilt) {
                        tilt_accum += m_tilt;
                    }
                }

                if (nextOffset == 0) {
                    loop = 4;
                    break;
                }
                m_sprdline = (m_sprdline + nextOffset) & 0xFFFF;
            }
        }

        quadrant = (quadrant + 1) & 0x03;
    }
}

uint16_t Suzy::initSpriteLine(uint16_t addr, bool literalMode, int pixelBits, int voff)
{
    m_line_shift_reg = 0;
    m_line_shift_reg_count = 0;
    m_line_repeat_count = 0;
    m_line_pixel = 0;
    m_line_type = LINE_ERROR;
    m_line_packet_bits_left = 0xFFFF;
    m_line_data_ptr = addr;

    int offset = getSpriteLineBits(8);
    if (offset <= 1) {
        m_line_packet_bits_left = 0;
    } else {
        m_line_packet_bits_left = (offset - 1) * 8;
    }

    if (literalMode) {
        m_line_type = LINE_ABS_LITERAL;
        if (offset > 1) {
            m_line_repeat_count = ((offset - 1) * 8) / pixelBits;
        }
    }

    if (voff < 0) voff = 0;
    if (voff > 101) voff = 101;
    m_line_base_address = m_vidbas + voff * 80;
    m_line_collision_address = m_collbas + voff * 80;
    return static_cast<uint16_t>(offset);
}

int Suzy::getSpriteLineBits(int bits)
{
    if (bits <= 0 || bits > 32) return 0;
    if (m_line_packet_bits_left <= bits) return 0;

    while (m_line_shift_reg_count < bits) {
        m_line_shift_reg <<= 8;
        m_line_shift_reg |= m_ram[m_line_data_ptr & 0xFFFF];
        m_line_data_ptr = (m_line_data_ptr + 1) & 0xFFFF;
        m_line_shift_reg_count += 8;
    }

    int shift = m_line_shift_reg_count - bits;
    int value = (m_line_shift_reg >> shift) & ((1 << bits) - 1);
    m_line_shift_reg_count -= bits;
    m_line_packet_bits_left -= bits;
    return value;
}

int Suzy::getSpriteLinePixel(int pixelBits)
{
    if (m_line_repeat_count == 0) {
        if (m_line_type != LINE_ABS_LITERAL) {
            int literal = getSpriteLineBits(1);
            m_line_type = literal ? LINE_LITERAL : LINE_PACKED;
        }

        switch (m_line_type) {
        case LINE_ABS_LITERAL:
            m_line_pixel = LINE_END;
            return m_line_pixel;
        case LINE_LITERAL:
            m_line_repeat_count = getSpriteLineBits(4) + 1;
            break;
        case LINE_PACKED:
            m_line_repeat_count = getSpriteLineBits(4);
            if (m_line_repeat_count == 0) {
                m_line_pixel = LINE_END;
                return m_line_pixel;
            }
            m_line_pixel = m_pen_index[getSpriteLineBits(pixelBits) & 0x0F];
            m_line_repeat_count++;
            break;
        default:
            return LINE_END;
        }
    }

    if (m_line_pixel != LINE_END) {
        m_line_repeat_count--;
        switch (m_line_type) {
        case LINE_ABS_LITERAL: {
            int pixel = getSpriteLineBits(pixelBits);
            if (m_line_repeat_count == 0 && pixel == 0) {
                m_line_pixel = LINE_END;
            } else {
                m_line_pixel = m_pen_index[pixel & 0x0F];
            }
            break;
        }
        case LINE_LITERAL:
            m_line_pixel = m_pen_index[getSpriteLineBits(pixelBits) & 0x0F];
            break;
        case LINE_PACKED:
            break;
        default:
            return LINE_END;
        }
    }

    return m_line_pixel;
}

void Suzy::plotPixel(int x, int y, uint8_t color, uint8_t collisionNum)
{
    if (!m_ram) return;
    if (x < 0 || x >= 160 || y < 0 || y >= 102) return;

    static int s_pixel_log = 0;
    if (s_pixel_log < 5) {
        s_pixel_log++;
        qDebug() << "plotPixel: x=" << x << "y=" << y
                 << "color=" << color << "vidbas=$" << Qt::hex << m_vidbas;
    }

    // Video buffer: each byte holds 2 pixels (4bpp) at 160x102
    // Buffer organization: 160 pixels / 2 = 80 bytes per line
    int offset = m_vidbas + y * 80 + x / 2;
    if (offset >= 0x10000) return;

    uint8_t existing = m_ram[offset & 0xFFFF];
    if (x & 1) {
        // Low nibble
        m_ram[offset & 0xFFFF] = (existing & 0xF0) | (color & 0x0F);
    } else {
        // High nibble
        m_ram[offset & 0xFFFF] = (existing & 0x0F) | ((color & 0x0F) << 4);
    }

    // Collision buffer (same layout as video buffer)
    if (m_collbas != 0 && collisionNum != 0) {
        int collOffset = m_collbas + y * 80 + x / 2;
        if (collOffset < 0x10000) {
            uint8_t coll = m_ram[collOffset & 0xFFFF];
            if (x & 1)
                m_ram[collOffset & 0xFFFF] = (coll & 0xF0) | (collisionNum & 0x0F);
            else
                m_ram[collOffset & 0xFFFF] = (coll & 0x0F) | ((collisionNum & 0x0F) << 4);
        }
    }
}

// ── Math coprocessor ──

void Suzy::doMathMultiply()
{
    // 16x16 → 32 multiply
    // Inputs: AB (16-bit) × CD (16-bit)
    // Result in EFGH (32-bit)
    uint16_t ab, cd;
    uint32_t result;

    if (m_math_sign) {
        int16_t sab = (int16_t)((m_mathA << 8) | m_mathB);
        int16_t scd = (int16_t)((m_mathC << 8) | m_mathD);
        int32_t sresult = (int32_t)sab * scd;
        result = (uint32_t)sresult;
    } else {
        ab = (uint16_t(m_mathA) << 8) | m_mathB;
        cd = (uint16_t(m_mathC) << 8) | m_mathD;
        result = (uint32_t)ab * cd;
    }

    if (m_math_accumulate) {
        uint32_t existing = (uint32_t(m_mathE) << 24) | (uint32_t(m_mathF) << 16) |
                           (uint32_t(m_mathG) << 8) | m_mathH;
        result += existing;
    }

    m_mathE = (result >> 24) & 0xFF;
    m_mathF = (result >> 16) & 0xFF;
    m_mathG = (result >> 8) & 0xFF;
    m_mathH = result & 0xFF;
}

void Suzy::doMathDivide()
{
    // 32/16 → 16 quotient + 16 remainder
    // Dividend: EFGH (32-bit), Divisor: NP (16-bit)
    // Quotient → AB, Remainder → CD
    uint32_t dividend = (uint32_t(m_mathE) << 24) | (uint32_t(m_mathF) << 16) |
                        (uint32_t(m_mathG) << 8) | m_mathH;
    uint16_t divisor = (uint16_t(m_mathN) << 8) | m_mathP;

    if (divisor == 0) {
        // Division by zero: result is all 1s
        m_mathA = 0xFF;
        m_mathB = 0xFF;
        m_mathC = 0xFF;
        m_mathD = 0xFF;
        return;
    }

    uint32_t quotient = dividend / divisor;
    uint32_t remainder = dividend % divisor;

    m_mathA = (quotient >> 8) & 0xFF;
    m_mathB = quotient & 0xFF;
    m_mathC = (remainder >> 8) & 0xFF;
    m_mathD = remainder & 0xFF;
}

void Suzy::serialize(QDataStream &ds) const
{
    ds.writeRawData(reinterpret_cast<const char*>(m_regs), 256);
    ds << m_scb_next << m_vidbas << m_collbas;
    ds << m_sprite_processing << m_math_sign << m_math_accumulate;
    ds << m_mathA << m_mathB << m_mathC << m_mathD;
    ds << m_mathE << m_mathF << m_mathG << m_mathH;
    ds << m_mathN << m_mathP;
    ds << m_joystick << m_switches;
}

void Suzy::deserialize(QDataStream &ds)
{
    ds.readRawData(reinterpret_cast<char*>(m_regs), 256);
    ds >> m_scb_next >> m_vidbas >> m_collbas;
    ds >> m_sprite_processing >> m_math_sign >> m_math_accumulate;
    ds >> m_mathA >> m_mathB >> m_mathC >> m_mathD;
    ds >> m_mathE >> m_mathF >> m_mathG >> m_mathH;
    ds >> m_mathN >> m_mathP;
    ds >> m_joystick >> m_switches;
}
