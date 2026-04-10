#pragma once

#include <cstdint>
#include <algorithm>
#include <QImage>
#include <QDataStream>

class LynxCart;

// ════════════════════════════════════════════════════════════════════════
// Atari Lynx Mikey — Full chip emulation
// - 8 system timers (timer 0 = display line, timer 2 = audio sample rate base)
// - 4 LFSR-based audio channels with stereo output
// - Palette (16 colors, 12-bit RGB)
// - Display DMA (reads framebuffer from RAM, converts via palette)
// - Interrupt controller
// Register space: $FD00–$FDFF
// Correct register map (from Handy/lynxdef.h):
//   $FD00-$FD1F: Timers 0-7 (backup, ctla, cnt, ctlb x8)
//   $FD20-$FD3F: Audio 0-3 (vol, feedback, output, shift, backup, ctl, cnt, other x4)
//   $FD40-$FD44: ATTEN A-D, MPAN
//   $FD50: MSTEREO
//   $FD80: INTRST, $FD81: INTSET
//   $FD87: SYSCTL1, $FD88: MIKEYHREV
//   $FD8A: IODIR, $FD8B: IODAT
//   $FD8C: SERCTL, $FD8D: SERDAT
//   $FD90: SDONEACK, $FD91: CPUSLEEP
//   $FD92: DISPCTL, $FD93: PBKUP
//   $FD94-$FD95: DISPADR
//   $FDA0-$FDAF: GREEN palette
//   $FDB0-$FDBF: BLUERED palette
// ════════════════════════════════════════════════════════════════════════

class Mikey {
public:
    Mikey();

    void setRAM(uint8_t *ram) { m_ram = ram; }
    void setCart(LynxCart *cart) { m_cart = cart; }

    // Full register read/write for $FD00–$FDFF
    uint8_t read(uint16_t addr);
    void write(uint16_t addr, uint8_t val);

    // Advance all timers and audio by one master clock tick (16 MHz)
    void tick();

    // Check if any interrupt is pending (any bit set in status)
    bool irqPending() const { return m_interrupt_status != 0; }

    // Display: render current framebuffer from RAM to QImage
    QImage renderFrame() const;
    QImage buildFrame() const;

    // Display line interrupt fired (timer 0 underflow increments scanline)
    bool isFrameReady() const { return m_frame_ready; }
    void clearFrameReady() { m_frame_ready = false; }

    // Display accessors for debug
    uint16_t dispAddr() const { return m_disp_addr; }
    uint8_t dispctl() const { return m_dispctl; }
    uint32_t paletteRGB(int idx) const { return (idx >= 0 && idx < 16) ? m_palette_rgb[idx] : 0; }

    // MAPCTL register (controls memory mapping — read by LynxMemory)
    uint8_t mapctl() const { return m_mapctl; }

    // Pull model for SDL audio callback (stereo interleaved: L,R,L,R,…)
    int readAudioBuffer(int16_t* dest, int maxSamples);
    void setAudioSampleRate(int rate) { m_audio_sample_rate = rate; }

    void serialize(QDataStream &ds) const;
    void deserialize(QDataStream &ds);

private:
    uint8_t *m_ram = nullptr;
    LynxCart *m_cart = nullptr;

    // ── System timers (0–7) ──
    struct Timer {
        uint8_t backup  = 0;  // reload value
        uint8_t control = 0;  // prescaler, enable, interrupt enable, reset-done, etc.
        uint8_t counter = 0;  // current count
        uint8_t staticVal = 0; // static control
        bool    borrowFlag = false;
        int     prescaleCounter = 0;
    };
    Timer m_timers[8];

    void tickTimer(int idx);
    void decrementTimer(int idx);
    static int timerPrescalerDivisor(uint8_t control);

    // ── Interrupt controller ──
    // INTRST ($FD80): write 1s to clear bits
    // INTSET ($FD81): write 1s to set bits, read returns status
    uint8_t m_interrupt_status = 0;
    uint8_t m_timer_irq_mask = 0;  // bit N = timer N interrupt enabled (from CTLA bit 7)

    // ── Display ──
    uint8_t m_dispctl = 0;  // DISPCTL at $FD92
    uint8_t m_pbkup = 0;    // PBKUP at $FD93
    int m_current_scanline = 0;
    bool m_frame_ready = false;
    uint16_t m_disp_addr = 0; // DISPADR at $FD94-$FD95
    QImage m_last_frame;

    // ── Palette ── (correct layout: GREEN at $FDA0-AF, BLUERED at $FDB0-BF)
    uint8_t m_palette_green[16] = {}; // $FDA0-$FDAF
    uint8_t m_palette_br[16] = {};    // $FDB0-$FDBF (blue high nibble, red low nibble)
    uint32_t m_palette_rgb[16] = {};  // converted ARGB32 for QImage
    void updatePaletteEntry(int idx);

    // ── MAPCTL ──
    uint8_t m_mapctl = 0; // $FFF9

    // ── GPIO (IODIR at $FD8A, IODAT at $FD8B) ──
    uint8_t m_iodir = 0;
    uint8_t m_iodat = 0;
    void updateCartLines(); // propagate IODAT bits to cart strobe/data

    // ── SYSCTL1 at $FD87 ──
    uint8_t m_sysctl1 = 0;

    // ── Serial (SERCTL at $FD8C, SERDAT at $FD8D) ──
    uint8_t m_serctl = 0;
    uint8_t m_serdat = 0;

    // ── Audio channels (4 LFSR-based) ──
    struct AudioChannel {
        int8_t   volume   = 0;
        uint8_t  feedback = 0;
        int8_t   output   = 0;
        uint8_t  backup   = 0;
        uint8_t  control  = 0;
        uint8_t  counter  = 0;
        uint8_t  other    = 0;
        uint16_t shiftRegister = 0x001;
        bool     timerEnabled  = false;
        int      prescaleCounter = 0;
    };
    AudioChannel m_channels[4];

    void clockAudioChannel(int ch);
    int computeFeedback(const AudioChannel& ch) const;
    static int audioPrescalerDivisor(uint8_t control);

    // Audio output buffer (stereo interleaved)
    static constexpr int AUDIO_BUFFER_SIZE = 8192;
    int16_t m_audio_buffer[AUDIO_BUFFER_SIZE]{};
    int m_audio_buffer_write = 0;
    int m_audio_buffer_read  = 0;
    int    m_audio_sample_rate = 44100;
    double m_audio_resample_acc = 0.0;
    static constexpr int AUDIO_TICK_DIVIDER = 1000;
    int m_audio_tick_counter = 0;
    void generateSample();

    // General register storage for reads
    uint8_t m_regs[256] = {};
};
