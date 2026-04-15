#pragma once

#include <cstdint>
#include <QImage>
#include <QDataStream>
#include <QColor>

// Atari 2600 TIA — cycle-precise scanline renderer
// NTSC: 228 color clocks per scanline, 262 scanlines per frame
// PAL:  228 color clocks per scanline, 312 scanlines per frame
// SECAM: 228 color clocks per scanline, 312 scanlines per frame
// Visible area: 160 pixels wide (68–227 color clocks)

enum class TVStandard { NTSC, PAL, SECAM };

class TIA {
public:
    TIA();
    ~TIA();

    uint8_t read(uint16_t addr) const;
    void write(uint16_t addr, uint8_t val);

    // Call per CPU cycle (1 CPU cycle = 3 color clocks)
    void tick(int cpuCycles);

    // Single color-clock advance (used for precise WSYNC handling)
    void tickSingleClock();

    int getColorClock() const { return colorClock; }

    QImage getFrame() const { return frame; }
    // Legacy compat — returns current frame
    QImage renderFrame() { return frame; }

    bool isFrameReady() const { return frameReady; }
    void clearFrameReady() { frameReady = false; }

    // WSYNC: CPU should halt until end of current scanline
    bool wsyncHalt() const { return m_wsync; }
    void clearWsync() { m_wsync = false; }

    // Input latches (active low — 0x00 = pressed, 0x80 = released)
    void setFireButton(int player, bool pressed) {
        if (player == 0) inpt4 = pressed ? 0x00 : 0x80;
        else             inpt5 = pressed ? 0x00 : 0x80;
    }

    // ── Audio ──
    // Called by EmulatorCore to pull generated audio samples
    int readAudioBuffer(int16_t* dest, int maxSamples);
    void setAudioSampleRate(int rate) { m_audio_sample_rate = rate; }

    // TV standard (palette + timing)
    void setTVStandard(TVStandard std);
    TVStandard getTVStandard() const { return tvStandard; }
    int scanlinesPerFrame() const;

    void serialize(QDataStream &ds) const;
    void deserialize(QDataStream &ds);

    // Palettes (128 colors each, indexed by high 7 bits of color value)
    static const uint32_t ntscPalette[128];
    static const uint32_t palPalette[128];
    static const uint32_t secamPalette[128];

private:
    // ── Registers (active copies) ──
    // Playfield
    uint8_t PF0, PF1, PF2;
    bool REF;       // playfield reflect mode (CTRLPF bit 0)
    bool SCORE;     // score mode (CTRLPF bit 1)
    bool PFP;       // playfield priority (CTRLPF bit 2)
    uint8_t COLUPF; // playfield color
    uint8_t COLUBK; // background color

    // Players
    uint8_t GRP0, GRP1;       // graphics
    uint8_t COLUP0, COLUP1;   // colors
    int16_t posP0, posP1;     // horizontal positions (0..159)
    uint8_t NUSIZ0, NUSIZ1;   // number-size
    bool REFP0, REFP1;        // reflect player
    uint8_t VDELP0, VDELP1;   // vertical delay
    uint8_t oldGRP0, oldGRP1; // previous GRP for VDEL

    // Missiles
    uint8_t ENAM0, ENAM1;
    int16_t posM0, posM1;
    uint8_t sizM0, sizM1; // derived from NUSIZ
    bool RESMP0 = false, RESMP1 = false; // missile reset-to-player flags

    // Ball
    uint8_t ENABL;
    int16_t posBL;
    uint8_t sizBL;   // from CTRLPF bits 4-5
    uint8_t VDELBL;
    uint8_t oldENABL;

    // HM registers (signed motion values, -8..+7)
    int8_t HMP0, HMP1, HMM0, HMM1, HMBL;

    // Collision latches (CXxx read registers)
    uint8_t collisions[8]; // CXM0P, CXM1P, CXP0FB, CXP1FB, CXM0FB, CXM1FB, CXBLPF, CXPPMM

    // ── Timing ──
    int colorClock;       // 0..227 within current scanline
    int scanline;         // 0..261 (NTSC) or 0..311 (PAL/SECAM)
    bool m_wsync;
    bool inVBlank;
    bool inVSync;
    bool m_hmoveBlank = false; // HMOVE blank: first 8 visible pixels forced to background
    bool m_vsyncFinalized = false; // true = frame already finalized by VSYNC this cycle
    TVStandard tvStandard = TVStandard::NTSC;

    // Visible-area tracking (auto-detected per frame)
    int visibleTop;   // first scanline with rendered pixels
    int visibleBottom; // last scanline with rendered pixels
    const uint32_t *activePalette = ntscPalette; // points to current palette

    // Input latches
    uint8_t inpt4 = 0x80; // P0 fire button (0x80 = not pressed)
    uint8_t inpt5 = 0x80; // P1 fire button

public:
    mutable uint8_t lastDataBus = 0; // last value on data bus for open-bus reads

private:

    // ── Frame buffer ──
    QImage frame;
    QImage backBuffer;
    bool frameReady;

    // ── Internals ──
    void clockTick();          // advance one color clock
    void renderPixel(int cc);  // render one visible pixel
    uint32_t colorLookup(uint8_t colReg) const;
    void audioTick();          // advance audio by one TIA audio clock

    // Playfield bit at pixel position 0..159
    bool pfBit(int px) const;

    // Player/missile/ball pixel test at screen x (0..159)
    bool playerPixel(int px, int16_t pos, uint8_t grp, bool ref, uint8_t nusiz) const;
    int missileWidth(uint8_t nusiz) const;
    int ballWidth() const;

    // Collision detection helpers
    void updateCollisions(bool p0, bool p1, bool m0, bool m1, bool bl, bool pf);

    // ── Audio registers ──
    struct AudioChannel {
        uint8_t AUDC = 0;   // control (waveform), 4 bits
        uint8_t AUDF = 0;   // frequency divider, 5 bits
        uint8_t AUDV = 0;   // volume, 4 bits
        // Internal state
        uint8_t  divCounter = 0;  // frequency divider counter
        uint8_t  polyCounter = 0; // polynomial counter (5-bit for LFSR)
        uint32_t poly4 = 0x000F;  // 4-bit LFSR
        uint32_t poly5 = 0x001F;  // 5-bit LFSR
        uint32_t poly9 = 0x01FF;  // 9-bit LFSR
        bool     outBit = false;  // current output bit
    };
    AudioChannel m_audio[2];

    // Audio sample accumulation
    int m_audio_clock_counter = 0;     // color clocks since last audio tick (ticks every 114 CC)
    int m_audio_sample_rate = 44100;   // output sample rate
    double m_audio_resample_acc = 0.0; // fractional accumulator for resampling
    static const int AUDIO_BUFFER_SIZE = 4096;
    int16_t m_audio_buffer[AUDIO_BUFFER_SIZE]{};
    int m_audio_buffer_write = 0;
    int m_audio_buffer_read = 0;
};
