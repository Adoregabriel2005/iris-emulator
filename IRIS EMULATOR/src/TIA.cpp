#include "TIA.h"
#include <QDebug>
#include <cstring>

// ════════════════════════════════════════════════════════════════════════
// NTSC palette — 128 entries (indexed by bits 7-1 of the color register)
// Each entry is 0x00RRGGBB.  Derived from standard Atari 2600 NTSC palette.
// ════════════════════════════════════════════════════════════════════════
const uint32_t TIA::ntscPalette[128] = {
    // Lum 0-7 for each of 16 hues
    // Hue 0 (grey)
    0x000000, 0x1A1A1A, 0x393939, 0x5B5B5B, 0x7E7E7E, 0xA2A2A2, 0xC7C7C7, 0xEDEDED,
    // Hue 1 (gold)
    0x190200, 0x3A1F00, 0x5D4100, 0x7B5904, 0x9C7A0A, 0xBB9830, 0xDCB959, 0xFEDB85,
    // Hue 2 (orange)
    0x2B0000, 0x4C1700, 0x6D3500, 0x8B5200, 0xAD710E, 0xCC8F32, 0xEDB05A, 0xFFD084,
    // Hue 3 (red-orange)
    0x350000, 0x570E00, 0x792D00, 0x984C00, 0xB86B13, 0xD88939, 0xF8AA60, 0xFFC98A,
    // Hue 4 (pink)
    0x340000, 0x560100, 0x781D16, 0x983E38, 0xB95F5C, 0xD97E7F, 0xF99FA3, 0xFFC0C7,
    // Hue 5 (purple)
    0x280000, 0x490013, 0x6B0E36, 0x8B2E58, 0xAC507B, 0xCC709D, 0xEC91BF, 0xFFB2E1,
    // Hue 6 (purple-blue)
    0x160010, 0x370031, 0x590E54, 0x792E76, 0x9A5098, 0xBA70BA, 0xDA91DC, 0xFBB2FE,
    // Hue 7 (blue)
    0x040024, 0x250246, 0x471868, 0x67388B, 0x8859AD, 0xA879CF, 0xC89AF1, 0xE8BBFF,
    // Hue 8 (blue)
    0x000034, 0x0E0F55, 0x2B2E76, 0x4B4E98, 0x6D6FBA, 0x8D8FDC, 0xADB0FE, 0xCDD1FF,
    // Hue 9 (light blue)
    0x00003A, 0x001A5B, 0x0F3A7D, 0x2F5A9E, 0x507BC0, 0x709BE2, 0x90BCFF, 0xB0DDFF,
    // Hue 10 (turquoise)
    0x000E34, 0x002C55, 0x004C77, 0x186C98, 0x398DBA, 0x59ADDC, 0x79CEFE, 0x99EEFF,
    // Hue 11 (green-blue)
    0x001E22, 0x003D43, 0x005D64, 0x107D86, 0x319EA7, 0x51BEC9, 0x71DFEB, 0x91FFFF,
    // Hue 12 (green)
    0x002A0A, 0x00492B, 0x056A4C, 0x258A6D, 0x46AB8E, 0x66CBB0, 0x86ECD1, 0xA6FFF3,
    // Hue 13 (yellow-green)
    0x003000, 0x004E12, 0x106E33, 0x308E54, 0x51AF76, 0x71CF97, 0x91F0B9, 0xB1FFDA,
    // Hue 14 (green)
    0x002E00, 0x0E4C00, 0x2F6C10, 0x4F8C31, 0x70AD52, 0x90CD73, 0xB0EE94, 0xD0FFB5,
    // Hue 15 (yellow-green)
    0x0A2400, 0x2B4200, 0x4C6200, 0x6C8217, 0x8DA339, 0xADC35A, 0xCDE47B, 0xEEFF9C,
};

// ════════════════════════════════════════════════════════════════════════
// PAL palette — 128 entries.  Derived from standard Atari 2600 PAL palette.
// PAL uses different hue mapping than NTSC (phase alternation).
// ════════════════════════════════════════════════════════════════════════
const uint32_t TIA::palPalette[128] = {
    // Hue 0 (grey)
    0x000000, 0x2B2B2B, 0x525252, 0x767676, 0x979797, 0xB6B6B6, 0xD2D2D2, 0xECECEC,
    // Hue 1 (gold/yellow)
    0x000000, 0x2B2B2B, 0x525252, 0x767676, 0x979797, 0xB6B6B6, 0xD2D2D2, 0xECECEC,
    // Hue 2 (yellow)
    0x805800, 0x966D00, 0xAB8300, 0xBF9800, 0xD2AD1A, 0xE4C23A, 0xF5D658, 0xFEEB75,
    // Hue 3 (yellow-green)
    0x445C00, 0x5E7500, 0x778D00, 0x90A500, 0xA9BD00, 0xC1D40C, 0xD8EB2C, 0xEFFF4A,
    // Hue 4 (green)
    0x0E5E00, 0x2C7800, 0x499000, 0x66A800, 0x83BF17, 0x9FD636, 0xBBEC54, 0xD6FF71,
    // Hue 5 (green-cyan)
    0x005800, 0x007000, 0x008A00, 0x0FA200, 0x30BA18, 0x4FD138, 0x6EE856, 0x8CFF73,
    // Hue 6 (cyan)
    0x004800, 0x005D04, 0x007626, 0x008F48, 0x18A86A, 0x38C08B, 0x56D7AB, 0x73EEC9,
    // Hue 7 (cyan-blue)
    0x002D40, 0x004260, 0x005A82, 0x0073A3, 0x188CC4, 0x38A4E4, 0x56BCFF, 0x73D3FF,
    // Hue 8 (blue)
    0x001064, 0x002884, 0x0042A5, 0x0B5CC5, 0x2C76E4, 0x4B90FF, 0x69AAFF, 0x86C3FF,
    // Hue 9 (blue-purple)
    0x1A0072, 0x321092, 0x4C2CB2, 0x6648D0, 0x8064EE, 0x997FFF, 0xB29AFF, 0xCAB4FF,
    // Hue 10 (purple)
    0x380064, 0x500A84, 0x6828A4, 0x8044C2, 0x9860E0, 0xAF7CFE, 0xC698FF, 0xDCB3FF,
    // Hue 11 (purple-magenta)
    0x52004A, 0x6A0C6A, 0x822A8A, 0x9A48A8, 0xB264C6, 0xCA80E4, 0xE09CFF, 0xF6B8FF,
    // Hue 12 (magenta)
    0x620028, 0x7A1248, 0x922E68, 0xAA4A88, 0xC266A8, 0xDA82C6, 0xF09EE4, 0xFFBAFF,
    // Hue 13 (magenta-red)
    0x640004, 0x7C1824, 0x943444, 0xAC5064, 0xC46C84, 0xDC88A4, 0xF4A4C2, 0xFFC0E0,
    // Hue 14 (red)
    0x581400, 0x703014, 0x884C34, 0xA06854, 0xB88474, 0xD0A094, 0xE8BCB4, 0xFFD8D2,
    // Hue 15 (red-orange)
    0x402C00, 0x584800, 0x706418, 0x888038, 0xA09C58, 0xB8B878, 0xD0D498, 0xE8F0B6,
};

// ════════════════════════════════════════════════════════════════════════
// SECAM palette — 8 colors only (SECAM has fixed color per register value).
// The Atari 2600 SECAM version maps colors differently: only 8 distinct colors.
// We map all 128 entries by cycling through the 8 SECAM colors.
// ════════════════════════════════════════════════════════════════════════
const uint32_t TIA::secamPalette[128] = {
    // SECAM colors: black, blue, red, magenta, green, cyan, yellow, white
    // Each luminance group maps to one of 8 colors; we repeat for all hues
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
    0x000000, 0x2121FF, 0xFF2121, 0xFF50FF, 0x21B921, 0x21FFFF, 0xFFFF21, 0xFFFFFF,
};

// ════════════════════════════════════════════════════════════════════════

TIA::TIA()
{
    frame      = QImage(160, 312, QImage::Format_RGB32);  // max PAL height
    backBuffer = QImage(160, 312, QImage::Format_RGB32);
    frame.fill(Qt::black);
    backBuffer.fill(Qt::black);
    visibleTop = 999;
    visibleBottom = 0;

    PF0 = PF1 = PF2 = 0;
    REF = SCORE = PFP = false;
    COLUPF = COLUBK = 0;

    GRP0 = GRP1 = 0;
    COLUP0 = COLUP1 = 0;
    posP0 = posP1 = 0;
    NUSIZ0 = NUSIZ1 = 0;
    REFP0 = REFP1 = false;
    VDELP0 = VDELP1 = 0;
    oldGRP0 = oldGRP1 = 0;

    ENAM0 = ENAM1 = 0;
    posM0 = posM1 = 0;
    sizM0 = sizM1 = 1;
    RESMP0 = RESMP1 = false;

    ENABL = 0;
    posBL = 0;
    sizBL = 1;
    VDELBL = 0;
    oldENABL = 0;

    HMP0 = HMP1 = HMM0 = HMM1 = HMBL = 0;

    memset(collisions, 0, sizeof(collisions));

    colorClock = 0;
    scanline   = 0;
    m_wsync    = false;
    inVBlank   = true;
    inVSync    = false;
    m_hmoveBlank = false;
    m_vsyncFinalized = false;
    frameReady = false;
    tvStandard = TVStandard::NTSC;
    activePalette = ntscPalette;
}

TIA::~TIA() {}

// ════════════════════════════════════════════════════════════════════════
// TV standard switching
// ════════════════════════════════════════════════════════════════════════
void TIA::setTVStandard(TVStandard std)
{
    tvStandard = std;
    switch (std) {
    case TVStandard::NTSC:  activePalette = ntscPalette;  break;
    case TVStandard::PAL:   activePalette = palPalette;   break;
    case TVStandard::SECAM: activePalette = secamPalette; break;
    }
}

int TIA::scanlinesPerFrame() const
{
    return (tvStandard == TVStandard::NTSC) ? 262 : 312;
}

// ════════════════════════════════════════════════════════════════════════
// Register read (active input ports + collision latches)
// ════════════════════════════════════════════════════════════════════════
uint8_t TIA::read(uint16_t addr) const
{
    uint8_t reg = addr & 0x0F;
    switch (reg) {
    // Collision registers: D7-D6 are valid, D5-D0 are open bus
    case 0x00: return (collisions[0] & 0xC0) | (lastDataBus & 0x3F); // CXM0P
    case 0x01: return (collisions[1] & 0xC0) | (lastDataBus & 0x3F); // CXM1P
    case 0x02: return (collisions[2] & 0xC0) | (lastDataBus & 0x3F); // CXP0FB
    case 0x03: return (collisions[3] & 0xC0) | (lastDataBus & 0x3F); // CXP1FB
    case 0x04: return (collisions[4] & 0xC0) | (lastDataBus & 0x3F); // CXM0FB
    case 0x05: return (collisions[5] & 0xC0) | (lastDataBus & 0x3F); // CXM1FB
    case 0x06: return (collisions[6] & 0x80) | (lastDataBus & 0x7F); // CXBLPF (D7 only)
    case 0x07: return (collisions[7] & 0xC0) | (lastDataBus & 0x3F); // CXPPMM
    case 0x08: case 0x09: case 0x0A: case 0x0B:
        return 0x80 | (lastDataBus & 0x7F); // INPT0-INPT3 (paddle) — D7 only
    case 0x0C: return (inpt4 & 0x80) | (lastDataBus & 0x7F); // INPT4 (P0 fire)
    case 0x0D: return (inpt5 & 0x80) | (lastDataBus & 0x7F); // INPT5 (P1 fire)
    default: return lastDataBus;
    }
}

// ════════════════════════════════════════════════════════════════════════
// Register write
// ════════════════════════════════════════════════════════════════════════
void TIA::write(uint16_t addr, uint8_t val)
{
    uint8_t reg = addr & 0x3F;
    switch (reg) {
    // ── Sync / blank ──
    case 0x00: // VSYNC
        if ((val & 0x02) && !inVSync) {
            // VSYNC asserted — finalize frame with whatever scanlines we have
            int top = visibleTop;
            int bot = visibleBottom;
            if (top > bot || top >= backBuffer.height()) {
                frame = QImage(160, 192, QImage::Format_RGB32);
                frame.fill(qRgb(0, 0, 0));
            } else {
                int h = bot - top + 1;
                frame = backBuffer.copy(0, top, 160, h);
            }
            backBuffer.fill(qRgb(0, 0, 0));
            visibleTop = 999;
            visibleBottom = 0;
            frameReady = true;
            m_vsyncFinalized = true;
            scanline = 0;
        }
        inVSync = (val & 0x02) != 0;
        break;
    case 0x01: // VBLANK
        inVBlank = (val & 0x02) != 0;
        break;
    case 0x02: // WSYNC — halt CPU until end of scanline
        m_wsync = true;
        break;

    // ── Player / missile / ball resets ──
    case 0x03: // RSYNC (rarely used — reset horizontal counter)
        colorClock = 0;
        break;
    case 0x04: NUSIZ0 = val; sizM0 = 1 << ((val >> 4) & 0x03); break;
    case 0x05: NUSIZ1 = val; sizM1 = 1 << ((val >> 4) & 0x03); break;

    // ── Colors ──
    case 0x06: COLUP0 = val; break;
    case 0x07: COLUP1 = val; break;
    case 0x08: COLUPF = val; break;
    case 0x09: COLUBK = val; break;

    // ── Control / playfield ──
    case 0x0A: // CTRLPF
        REF   = (val & 0x01) != 0;
        SCORE = (val & 0x02) != 0;
        PFP   = (val & 0x04) != 0;
        sizBL = 1 << ((val >> 4) & 0x03);
        break;
    case 0x0B: REFP0 = (val & 0x08) != 0; break;
    case 0x0C: REFP1 = (val & 0x08) != 0; break;

    // ── Playfield ──
    case 0x0D: PF0 = val; break;
    case 0x0E: PF1 = val; break;
    case 0x0F: PF2 = val; break;

    // ── Position resets (strobe) ──
    // TIA hardware pipeline delay: +5 clocks for players, +4 for missiles/ball
    // Unified formula handles both HBLANK (CC<68) and visible area with wrap-around
    case 0x10: posP0 = ((colorClock - 68 + 5) % 160 + 160) % 160; break; // RESP0
    case 0x11: posP1 = ((colorClock - 68 + 5) % 160 + 160) % 160; break; // RESP1
    case 0x12: posM0 = ((colorClock - 68 + 4) % 160 + 160) % 160; break; // RESM0
    case 0x13: posM1 = ((colorClock - 68 + 4) % 160 + 160) % 160; break; // RESM1
    case 0x14: posBL = ((colorClock - 68 + 4) % 160 + 160) % 160; break; // RESBL

    // ── Audio ──
    case 0x15: m_audio[0].AUDC = val & 0x0F; break; // AUDC0
    case 0x16: m_audio[1].AUDC = val & 0x0F; break; // AUDC1
    case 0x17: m_audio[0].AUDF = val & 0x1F; break; // AUDF0
    case 0x18: m_audio[1].AUDF = val & 0x1F; break; // AUDF1
    case 0x19: m_audio[0].AUDV = val & 0x0F; break; // AUDV0
    case 0x1A: m_audio[1].AUDV = val & 0x0F; break; // AUDV1

    // ── Graphics ──
    case 0x1B: // GRP0
        oldGRP1 = GRP1; // writing GRP0 latches old GRP1
        GRP0 = val;
        break;
    case 0x1C: // GRP1
        oldGRP0 = GRP0; // writing GRP1 latches old GRP0
        oldENABL = ENABL; // writing GRP1 also latches old ENABL (for VDELBL)
        GRP1 = val;
        break;
    case 0x1D: ENAM0 = (val & 0x02) ? 1 : 0; break;
    case 0x1E: ENAM1 = (val & 0x02) ? 1 : 0; break;
    case 0x1F: // ENABL
        ENABL = (val & 0x02) ? 1 : 0;
        break;

    // ── Horizontal motion ──
    case 0x20: HMP0 = (int8_t)(val) >> 4; break;
    case 0x21: HMP1 = (int8_t)(val) >> 4; break;
    case 0x22: HMM0 = (int8_t)(val) >> 4; break;
    case 0x23: HMM1 = (int8_t)(val) >> 4; break;
    case 0x24: HMBL = (int8_t)(val) >> 4; break;

    // ── HMOVE — apply motion ──
    case 0x2A: {
        posP0 = (posP0 - HMP0 + 160) % 160;
        posP1 = (posP1 - HMP1 + 160) % 160;
        posM0 = (posM0 - HMM0 + 160) % 160;
        posM1 = (posM1 - HMM1 + 160) % 160;
        posBL = (posBL - HMBL + 160) % 160;
        // HMOVE blank: only produced when HMOVE is strobed during HBLANK (CC 0-67)
        if (colorClock < 68) {
            m_hmoveBlank = true;
        }
        break;
    }

    // ── HMCLR — clear motion registers ──
    case 0x2B:
        HMP0 = HMP1 = HMM0 = HMM1 = HMBL = 0;
        break;

    // ── CXCLR — clear collision latches ──
    case 0x2C:
        memset(collisions, 0, sizeof(collisions));
        break;

    // Vertical delay
    case 0x25: VDELP0 = val & 0x01; break;
    case 0x26: VDELP1 = val & 0x01; break;
    case 0x27: VDELBL = val & 0x01; break;

    // Missile reset to player (flag — missile follows player while set)
    case 0x28: RESMP0 = (val & 0x02) != 0; break;
    case 0x29: RESMP1 = (val & 0x02) != 0; break;

    default:
        break;
    }
}

// ════════════════════════════════════════════════════════════════════════
// Color lookup from register value using NTSC palette
// ════════════════════════════════════════════════════════════════════════
uint32_t TIA::colorLookup(uint8_t colReg) const
{
    return activePalette[(colReg >> 1) & 0x7F];
}

// ════════════════════════════════════════════════════════════════════════
// Playfield bit — returns true if PF is set at visible pixel x (0..159)
// PF is 20 bits wide across the left half (0..79). The right half (80..159)
// is either a mirror (REF) or a repeat.
// PF0 bits 4-7 (MSB first) → columns 0-3
// PF1 bits 7-0 (MSB first) → columns 4-11
// PF2 bits 0-7 (LSB first) → columns 12-19
// ════════════════════════════════════════════════════════════════════════
bool TIA::pfBit(int px) const
{
    int col;
    if (px < 80) {
        col = px / 4; // 0..19
    } else {
        if (REF)
            col = 19 - (px - 80) / 4; // mirrored
        else
            col = (px - 80) / 4; // repeated
    }

    if (col < 4) {
        // PF0 bits 4-7 (bit 4 = leftmost)
        return (PF0 >> (4 + col)) & 1;
    } else if (col < 12) {
        // PF1 bits 7-0 (bit 7 = leftmost = col 4)
        return (PF1 >> (7 - (col - 4))) & 1;
    } else {
        // PF2 bits 0-7 (bit 0 = leftmost = col 12)
        return (PF2 >> (col - 12)) & 1;
    }
}

// ════════════════════════════════════════════════════════════════════════
// Player pixel test — checks if player graphic covers pixel px
// Handles copies/sizes from NUSIZ lower 3 bits
// ════════════════════════════════════════════════════════════════════════

// NUSIZ copy offsets: start-to-start distances from player position
// -1 means "no copy at this slot"
static const int nusizCopyCount[8]     = {1, 2, 2, 3, 2, 1, 3, 1};
static const int nusizCopyOffset[8][3] = {
    {0, -1, -1},   // 0: one copy
    {0, 16, -1},   // 1: two copies, close (16 pixels apart)
    {0, 32, -1},   // 2: two copies, medium (32 pixels apart)
    {0, 16, 32},   // 3: three copies, close
    {0, 64, -1},   // 4: two copies, wide (64 pixels apart)
    {0, -1, -1},   // 5: double-size player
    {0, 32, 64},   // 6: three copies, medium
    {0, -1, -1},   // 7: quad-size player
};
static const int nusizStretch[8] = {1, 1, 1, 1, 1, 2, 1, 4};

bool TIA::playerPixel(int px, int16_t pos, uint8_t grp, bool ref, uint8_t nusiz) const
{
    if (grp == 0) return false;

    int idx = nusiz & 0x07;
    int stretch = nusizStretch[idx];
    int pwidth = 8 * stretch;
    int copies = nusizCopyCount[idx];

    for (int c = 0; c < copies; ++c) {
        int copyPos = (pos + nusizCopyOffset[idx][c]) % 160;
        if (copyPos < 0) copyPos += 160;
        int rel = (px - copyPos + 160) % 160;
        if (rel < pwidth) {
            int bit = rel / stretch;
            if (ref) bit = 7 - bit;
            if ((grp >> (7 - bit)) & 1)
                return true;
        }
    }
    return false;
}

int TIA::missileWidth(uint8_t nusiz) const
{
    return 1 << ((nusiz >> 4) & 0x03);
}

int TIA::ballWidth() const
{
    return sizBL;
}

// ════════════════════════════════════════════════════════════════════════
// Collision detection — called per visible pixel
// ════════════════════════════════════════════════════════════════════════
void TIA::updateCollisions(bool p0, bool p1, bool m0, bool m1, bool bl, bool pf)
{
    // CXM0P  (D7: M0-P1, D6: M0-P0)
    if (m0 && p1) collisions[0] |= 0x80;
    if (m0 && p0) collisions[0] |= 0x40;
    // CXM1P  (D7: M1-P0, D6: M1-P1)
    if (m1 && p0) collisions[1] |= 0x80;
    if (m1 && p1) collisions[1] |= 0x40;
    // CXP0FB (D7: P0-PF, D6: P0-BL)
    if (p0 && pf) collisions[2] |= 0x80;
    if (p0 && bl) collisions[2] |= 0x40;
    // CXP1FB (D7: P1-PF, D6: P1-BL)
    if (p1 && pf) collisions[3] |= 0x80;
    if (p1 && bl) collisions[3] |= 0x40;
    // CXM0FB (D7: M0-PF, D6: M0-BL)
    if (m0 && pf) collisions[4] |= 0x80;
    if (m0 && bl) collisions[4] |= 0x40;
    // CXM1FB (D7: M1-PF, D6: M1-BL)
    if (m1 && pf) collisions[5] |= 0x80;
    if (m1 && bl) collisions[5] |= 0x40;
    // CXBLPF (D7: BL-PF)
    if (bl && pf) collisions[6] |= 0x80;
    // CXPPMM (D7: P0-P1, D6: M0-M1)
    if (p0 && p1) collisions[7] |= 0x80;
    if (m0 && m1) collisions[7] |= 0x40;
}

// ════════════════════════════════════════════════════════════════════════
// Render one visible pixel at the current color clock position
// ════════════════════════════════════════════════════════════════════════
void TIA::renderPixel(int cc)
{
    int px = cc - 68; // visible pixel 0..159
    if (px < 0 || px >= 160) return;
    if (scanline < 0 || scanline >= backBuffer.height()) return;

    // Track actual visible bounds
    if (scanline < visibleTop) visibleTop = scanline;
    if (scanline > visibleBottom) visibleBottom = scanline;

    // Determine active graphics
    uint8_t grp0use = VDELP0 ? oldGRP0 : GRP0;
    uint8_t grp1use = VDELP1 ? oldGRP1 : GRP1;
    uint8_t blUse   = VDELBL ? oldENABL : ENABL;

    bool pf = pfBit(px);
    bool p0 = playerPixel(px, posP0, grp0use, REFP0, NUSIZ0);
    bool p1 = playerPixel(px, posP1, grp1use, REFP1, NUSIZ1);

    // Missiles — use the same NUSIZ copy pattern as the parent player
    bool m0 = false, m1 = false;
    if (RESMP0) {
        // Lock missile to player center — offset depends on player width
        int stretch0 = nusizStretch[NUSIZ0 & 0x07];
        posM0 = (posP0 + 4 * stretch0 - 1) % 160;
    }
    if (ENAM0 && !RESMP0) {
        int w = missileWidth(NUSIZ0);
        int idx = NUSIZ0 & 0x07;
        int nc = nusizCopyCount[idx];
        for (int c = 0; c < nc; ++c) {
            int mp = (posM0 + nusizCopyOffset[idx][c]) % 160;
            if (mp < 0) mp += 160;
            int rel = (px - mp + 160) % 160;
            if (rel < w) { m0 = true; break; }
        }
    }
    if (RESMP1) {
        int stretch1 = nusizStretch[NUSIZ1 & 0x07];
        posM1 = (posP1 + 4 * stretch1 - 1) % 160;
    }
    if (ENAM1 && !RESMP1) {
        int w = missileWidth(NUSIZ1);
        int idx = NUSIZ1 & 0x07;
        int nc = nusizCopyCount[idx];
        for (int c = 0; c < nc; ++c) {
            int mp = (posM1 + nusizCopyOffset[idx][c]) % 160;
            if (mp < 0) mp += 160;
            int rel = (px - mp + 160) % 160;
            if (rel < w) { m1 = true; break; }
        }
    }

    // Ball
    bool bl = false;
    if (blUse) {
        int w = ballWidth();
        int rel = (px - posBL + 160) % 160;
        if (rel < w) bl = true;
    }

    // Collision detection (always runs, even during HMOVE blank)
    updateCollisions(p0, p1, m0, m1, bl, pf);

    // HMOVE blank: first 8 visible pixels forced to background color.
    // Hardware blanks these pixels when HMOVE is strobed; collisions still occur.
    if (m_hmoveBlank && px < 8) {
        backBuffer.setPixel(px, scanline, colorLookup(COLUBK));
        return;
    }

    // Priority compositing
    uint32_t color = colorLookup(COLUBK); // default: background

    if (PFP) {
        // Playfield priority (PF/BL over players)
        if (pf || bl) {
            color = colorLookup(COLUPF);
            if (SCORE && pf) {
                color = (px < 80) ? colorLookup(COLUP0) : colorLookup(COLUP1);
            }
        } else if (p0 || m0) {
            color = colorLookup(COLUP0);
        } else if (p1 || m1) {
            color = colorLookup(COLUP1);
        }
    } else {
        // Normal priority (players over PF)
        if (p0 || m0) {
            color = colorLookup(COLUP0);
        } else if (p1 || m1) {
            color = colorLookup(COLUP1);
        } else if (pf || bl) {
            color = colorLookup(COLUPF);
            if (SCORE && pf) {
                color = (px < 80) ? colorLookup(COLUP0) : colorLookup(COLUP1);
            }
        }
    }

    backBuffer.setPixel(px, scanline, color);
}

// ════════════════════════════════════════════════════════════════════════
// Advance one color clock
// ════════════════════════════════════════════════════════════════════════
void TIA::clockTick()
{
    // Object scanners run during all 228 color clocks for collision detection
    if (!inVBlank && !inVSync) {
        if (colorClock >= 68 && colorClock < 228) {
            // Visible area: render pixel + collisions
            renderPixel(colorClock);
        } else {
            // HBLANK (CC 0-67): still detect collisions but don't render
            int px = (colorClock - 68 + 160) % 160; // wrap for HBLANK region
            uint8_t grp0use = VDELP0 ? oldGRP0 : GRP0;
            uint8_t grp1use = VDELP1 ? oldGRP1 : GRP1;
            uint8_t blUse   = VDELBL ? oldENABL : ENABL;
            bool pf = pfBit(px);
            bool p0 = playerPixel(px, posP0, grp0use, REFP0, NUSIZ0);
            bool p1 = playerPixel(px, posP1, grp1use, REFP1, NUSIZ1);
            bool m0 = false, m1 = false, bl = false;
            if (ENAM0 && !RESMP0) {
                int w = missileWidth(NUSIZ0);
                int idx = NUSIZ0 & 0x07;
                int nc = nusizCopyCount[idx];
                for (int c = 0; c < nc; ++c) {
                    int mp = (posM0 + nusizCopyOffset[idx][c]) % 160;
                    if (mp < 0) mp += 160;
                    int rel = (px - mp + 160) % 160;
                    if (rel < w) { m0 = true; break; }
                }
            }
            if (ENAM1 && !RESMP1) {
                int w = missileWidth(NUSIZ1);
                int idx = NUSIZ1 & 0x07;
                int nc = nusizCopyCount[idx];
                for (int c = 0; c < nc; ++c) {
                    int mp = (posM1 + nusizCopyOffset[idx][c]) % 160;
                    if (mp < 0) mp += 160;
                    int rel = (px - mp + 160) % 160;
                    if (rel < w) { m1 = true; break; }
                }
            }
            if (blUse) {
                int w = ballWidth();
                int rel = (px - posBL + 160) % 160;
                if (rel < w) bl = true;
            }
            updateCollisions(p0, p1, m0, m1, bl, pf);
        }
    }

    // Audio: TIA audio clocks once every 114 color clocks (once per scanline half)
    // Actually: TIA divides its clock by 114 to get ~31440 Hz audio clock
    m_audio_clock_counter++;
    if (m_audio_clock_counter >= 114) {
        m_audio_clock_counter = 0;
        audioTick();
    }

    colorClock++;
    if (colorClock >= 228) {
        // End of scanline
        colorClock = 0;
        m_wsync = false; // release WSYNC at start of new scanline
        m_hmoveBlank = false; // HMOVE blank is per-scanline
        scanline++;

        if (scanline >= scanlinesPerFrame() && !m_vsyncFinalized) {
            // Safety: frame exceeded max scanlines without VSYNC
            int top = visibleTop;
            int bot = visibleBottom;
            if (top > bot || top >= scanlinesPerFrame()) {
                frame = QImage(160, 192, QImage::Format_RGB32);
                frame.fill(qRgb(0, 0, 0));
            } else {
                int h = bot - top + 1;
                frame = backBuffer.copy(0, top, 160, h);
            }
            backBuffer.fill(qRgb(0, 0, 0));
            scanline = 0;
            visibleTop = 999;
            visibleBottom = 0;
            frameReady = true;
        }
        // Reset flag once we pass the safety threshold
        if (scanline >= scanlinesPerFrame()) {
            m_vsyncFinalized = false;
            scanline = 0;
        }
    }
}

void TIA::tickSingleClock()
{
    clockTick();
}

// ════════════════════════════════════════════════════════════════════════
// tick() — called per CPU cycle (1 CPU cycle = 3 color clocks)
// ════════════════════════════════════════════════════════════════════════
void TIA::tick(int cpuCycles)
{
    int cc = cpuCycles * 3;
    for (int i = 0; i < cc; ++i) {
        clockTick();
    }
}

// ════════════════════════════════════════════════════════════════════════
// Audio — TIA sound generation
// The TIA audio clock runs at CPU_CLK/114 ≈ 31440 Hz.
// Each channel has a frequency divider (AUDF+1), a waveform selector (AUDC),
// and a volume (AUDV). We generate samples at the TIA audio rate, then
// resample to the output sample rate when pushing to the buffer.
// ════════════════════════════════════════════════════════════════════════

static bool clockLFSR4(uint32_t& lfsr)
{
    // 4-bit LFSR: taps at bits 0,1 (x^4 + x + 1), period 15
    bool bit = ((lfsr & 1) ^ ((lfsr >> 1) & 1)) != 0;
    lfsr = (lfsr >> 1) | (bit ? 0x08 : 0);
    return lfsr & 1;
}

static bool clockLFSR5(uint32_t& lfsr)
{
    // 5-bit LFSR: taps at bits 0,2 (x^5 + x^2 + 1), period 31
    bool bit = ((lfsr & 1) ^ ((lfsr >> 2) & 1)) != 0;
    lfsr = (lfsr >> 1) | (bit ? 0x10 : 0);
    return lfsr & 1;
}

static bool clockLFSR9(uint32_t& lfsr)
{
    // 9-bit LFSR: taps at bits 0,4 (x^9 + x^4 + 1), period 511
    bool bit = ((lfsr & 1) ^ ((lfsr >> 4) & 1)) != 0;
    lfsr = (lfsr >> 1) | (bit ? 0x100 : 0);
    return lfsr & 1;
}

void TIA::audioTick()
{
    // TIA audio rate = ~31440 Hz (3.58MHz / 114)
    const double TIA_AUDIO_RATE = 31440.0;

    int16_t mixedSample = 0;

    for (int ch = 0; ch < 2; ch++) {
        AudioChannel& c = m_audio[ch];

        // Frequency divider: counts down from AUDF, clocks waveform on underflow
        if (c.divCounter == 0) {
            c.divCounter = c.AUDF;

            // Clock the waveform generator based on AUDC
            switch (c.AUDC) {
            case 0x00: // Set to 1 (volume only)
                c.outBit = true;
                break;
            case 0x01: // 4-bit poly (period 15)
                c.outBit = clockLFSR4(c.poly4);
                break;
            case 0x02: // ÷31 → 4-bit poly (poly5 output gates poly4 clock)
                if (clockLFSR5(c.poly5))
                    c.outBit = clockLFSR4(c.poly4);
                break;
            case 0x03: // 5-bit poly → 4-bit poly
                if (clockLFSR5(c.poly5))
                    c.outBit = clockLFSR4(c.poly4);
                break;
            case 0x04: // Pure div 2 (square wave)
            case 0x05: // Pure div 2
                c.outBit = !c.outBit;
                break;
            case 0x06: // ÷31 pure tone (poly5 gates toggle)
            case 0x0A: // ÷31 pure tone
                if (clockLFSR5(c.poly5))
                    c.outBit = !c.outBit;
                break;
            case 0x07: // 5-bit poly → div 2
                if (clockLFSR5(c.poly5))
                    c.outBit = !c.outBit;
                break;
            case 0x08: // 9-bit poly (white noise)
                c.outBit = clockLFSR9(c.poly9);
                break;
            case 0x09: // 5-bit poly
                c.outBit = clockLFSR5(c.poly5);
                break;
            case 0x0B: // 5-bit poly (same output as 0x09)
                c.outBit = clockLFSR5(c.poly5);
                break;
            case 0x0C: // div 6 (÷3 counter with toggle)
            case 0x0D: // div 6
                c.polyCounter++;
                if (c.polyCounter >= 3) {
                    c.polyCounter = 0;
                    c.outBit = !c.outBit;
                }
                break;
            case 0x0E: // div 93 (poly5 gates ÷3 counter with toggle)
                if (clockLFSR5(c.poly5)) {
                    c.polyCounter++;
                    if (c.polyCounter >= 3) {
                        c.polyCounter = 0;
                        c.outBit = !c.outBit;
                    }
                }
                break;
            case 0x0F: // 5-bit poly → div 6 (poly5 gates ÷3 counter)
                if (clockLFSR5(c.poly5)) {
                    c.polyCounter++;
                    if (c.polyCounter >= 3) {
                        c.polyCounter = 0;
                        c.outBit = !c.outBit;
                    }
                }
                break;
            default:
                c.outBit = clockLFSR5(c.poly5);
                break;
            }
        } else {
            c.divCounter--;
        }

        // Mix: each channel outputs volume * output_bit
        int16_t val = c.outBit ? (c.AUDV * 2184) : 0; // scale 0-15 to ~0-32760
        mixedSample += val;
    }

    // Resample from TIA audio rate to output sample rate
    // We accumulate fractional samples to handle the rate conversion
    m_audio_resample_acc += (double)m_audio_sample_rate / TIA_AUDIO_RATE;
    while (m_audio_resample_acc >= 1.0) {
        m_audio_resample_acc -= 1.0;
        // Write to ring buffer
        int nextWrite = (m_audio_buffer_write + 1) % AUDIO_BUFFER_SIZE;
        if (nextWrite != m_audio_buffer_read) { // don't overwrite unread data
            m_audio_buffer[m_audio_buffer_write] = mixedSample;
            m_audio_buffer_write = nextWrite;
        }
    }
}

int TIA::readAudioBuffer(int16_t* dest, int maxSamples)
{
    int count = 0;
    while (count < maxSamples && m_audio_buffer_read != m_audio_buffer_write) {
        dest[count++] = m_audio_buffer[m_audio_buffer_read];
        m_audio_buffer_read = (m_audio_buffer_read + 1) % AUDIO_BUFFER_SIZE;
    }
    // Fill remainder with last sample (avoid clicks) or silence
    if (count > 0) {
        int16_t last = dest[count - 1];
        while (count < maxSamples)
            dest[count++] = last;
    } else {
        while (count < maxSamples)
            dest[count++] = 0;
    }
    return maxSamples;
}

// ════════════════════════════════════════════════════════════════════════
// Serialization
// ════════════════════════════════════════════════════════════════════════
void TIA::serialize(QDataStream &ds) const
{
    ds << PF0 << PF1 << PF2;
    ds << REF << SCORE << PFP;
    ds << COLUPF << COLUBK;
    ds << GRP0 << GRP1 << COLUP0 << COLUP1;
    ds << posP0 << posP1;
    ds << NUSIZ0 << NUSIZ1;
    ds << REFP0 << REFP1;
    ds << VDELP0 << VDELP1;
    ds << oldGRP0 << oldGRP1;
    ds << ENAM0 << ENAM1;
    ds << posM0 << posM1;
    ds << sizM0 << sizM1;
    ds << RESMP0 << RESMP1;
    ds << ENABL << posBL << sizBL;
    ds << VDELBL << oldENABL;
    ds << HMP0 << HMP1 << HMM0 << HMM1 << HMBL;
    for (int i = 0; i < 8; ++i) ds << collisions[i];
    ds << static_cast<qint32>(colorClock);
    ds << static_cast<qint32>(scanline);
    ds << m_wsync << inVBlank << inVSync << frameReady;
    ds << static_cast<qint32>(tvStandard);
    ds << static_cast<qint32>(visibleTop) << static_cast<qint32>(visibleBottom);
    ds << m_hmoveBlank;
}

void TIA::deserialize(QDataStream &ds)
{
    ds >> PF0 >> PF1 >> PF2;
    ds >> REF >> SCORE >> PFP;
    ds >> COLUPF >> COLUBK;
    ds >> GRP0 >> GRP1 >> COLUP0 >> COLUP1;
    ds >> posP0 >> posP1;
    ds >> NUSIZ0 >> NUSIZ1;
    ds >> REFP0 >> REFP1;
    ds >> VDELP0 >> VDELP1;
    ds >> oldGRP0 >> oldGRP1;
    ds >> ENAM0 >> ENAM1;
    ds >> posM0 >> posM1;
    ds >> sizM0 >> sizM1;
    ds >> RESMP0 >> RESMP1;
    ds >> ENABL >> posBL >> sizBL;
    ds >> VDELBL >> oldENABL;
    ds >> HMP0 >> HMP1 >> HMM0 >> HMM1 >> HMBL;
    for (int i = 0; i < 8; ++i) ds >> collisions[i];
    qint32 cc=0, sl=0;
    ds >> cc >> sl;
    colorClock = cc;
    scanline = sl;
    ds >> m_wsync >> inVBlank >> inVSync >> frameReady;
    qint32 tvs = 0;
    ds >> tvs;
    setTVStandard(static_cast<TVStandard>(tvs));
    qint32 vt = 999, vb = 0;
    ds >> vt >> vb;
    visibleTop = vt;
    visibleBottom = vb;
    if (ds.status() == QDataStream::Ok)
        ds >> m_hmoveBlank;
    else
        m_hmoveBlank = false;
}
