#include "Mikey.h"
#include "LynxCart.h"
#include <cstring>
#include <QDebug>

Mikey::Mikey()
{
    std::memset(m_regs, 0, sizeof(m_regs));
    std::memset(m_palette_green, 0, sizeof(m_palette_green));
    std::memset(m_palette_br, 0, sizeof(m_palette_br));

    for (int i = 0; i < 8; i++)
        m_timers[i] = {};

    for (int i = 0; i < 4; i++) {
        m_channels[i] = {};
        m_channels[i].shiftRegister = 0x001;
    }

    // Default palette: grayscale ramp
    for (int i = 0; i < 16; i++) {
        int gray = i * 17;
        m_palette_rgb[i] = 0xFF000000 | (gray << 16) | (gray << 8) | gray;
    }
}

// ════════════════════════════════════════════════════════════════════════
// Register read/write ($FD00–$FDFF) — Correct Lynx register map
// ════════════════════════════════════════════════════════════════════════

uint8_t Mikey::read(uint16_t addr)
{
    uint8_t reg = addr & 0xFF;

    // System timers $FD00–$FD1F (8 timers × 4 regs)
    if (reg < 0x20) {
        int tidx = reg >> 2;
        int treg = reg & 0x03;
        switch (treg) {
        case 0: return m_timers[tidx].backup;
        case 1: return m_timers[tidx].control;
        case 2: return m_timers[tidx].counter;
        case 3: return m_timers[tidx].staticVal;
        }
    }

    // Audio channels $FD20–$FD3F
    if (reg >= 0x20 && reg < 0x40) {
        int ch = (reg - 0x20) >> 3;
        int areg = (reg - 0x20) & 0x07;
        const auto& c = m_channels[ch];
        switch (areg) {
        case 0: return static_cast<uint8_t>(c.volume);
        case 1: return c.feedback;
        case 2: return static_cast<uint8_t>(c.output);
        case 3: return static_cast<uint8_t>(c.shiftRegister & 0xFF);
        case 4: return c.backup;
        case 5: return c.control;
        case 6: return c.counter;
        case 7: return (c.other & 0xF0) | ((c.shiftRegister >> 8) & 0x0F);
        }
    }

    // Attenuation & stereo
    if (reg >= 0x40 && reg <= 0x44) return m_regs[reg]; // ATTEN A-D, MPAN
    if (reg == 0x50) return m_regs[0x50]; // MSTEREO

    // Interrupt status — read INTSET ($FD81)
    if (reg == 0x80) return m_interrupt_status; // INTRST reads status too
    if (reg == 0x81) return m_interrupt_status; // INTSET

    // SYSCTL1
    if (reg == 0x87) return m_sysctl1;

    // Mikey revision
    if (reg == 0x88) return 0x01; // MIKEYHREV
    if (reg == 0x89) return 0x01; // MIKEYSREV

    // GPIO
    if (reg == 0x8A) return m_iodir;  // IODIR
    if (reg == 0x8B) return m_iodat;  // IODAT

    // Serial
    if (reg == 0x8C) return m_serctl; // SERCTL
    if (reg == 0x8D) return m_serdat; // SERDAT

    // SDONEACK
    if (reg == 0x90) return 0x00;

    // CPUSLEEP
    if (reg == 0x91) return 0x00;

    // Display control
    if (reg == 0x92) return m_dispctl; // DISPCTL
    if (reg == 0x93) return m_pbkup;   // PBKUP
    if (reg == 0x94) return static_cast<uint8_t>(m_disp_addr & 0xFF);      // DISPADRL
    if (reg == 0x95) return static_cast<uint8_t>((m_disp_addr >> 8) & 0xFF); // DISPADRH

    // Palette GREEN $FDA0–$FDAF
    if (reg >= 0xA0 && reg <= 0xAF)
        return m_palette_green[reg - 0xA0];

    // Palette BLUERED $FDB0–$FDBF
    if (reg >= 0xB0 && reg <= 0xBF)
        return m_palette_br[reg - 0xB0];

    // MAPCTL
    if (reg == 0xF9) return m_mapctl;

    return m_regs[reg];
}

void Mikey::write(uint16_t addr, uint8_t val)
{
    uint8_t reg = addr & 0xFF;
    m_regs[reg] = val;

    // System timers $FD00–$FD1F
    if (reg < 0x20) {
        int tidx = reg >> 2;
        int treg = reg & 0x03;
        switch (treg) {
        case 0: m_timers[tidx].backup = val; break;
        case 1: // CTLA
            m_timers[tidx].control = val;
            // Bit 7: interrupt enable → update mask
            if (val & 0x80)
                m_timer_irq_mask |= (1 << tidx);
            else
                m_timer_irq_mask &= ~(1 << tidx);
            // Bit 6: reset timer done
            if (val & 0x40)
                m_timers[tidx].borrowFlag = false;
            // Bit 4: reload counter from backup
            if (val & 0x10) {
                m_timers[tidx].counter = m_timers[tidx].backup;
                m_timers[tidx].prescaleCounter = 0;
            }
            break;
        case 2: m_timers[tidx].counter = val; break;
        case 3: m_timers[tidx].staticVal = val; break;
        }
        return;
    }

    // Audio channels $FD20–$FD3F
    if (reg >= 0x20 && reg < 0x40) {
        int ch = (reg - 0x20) >> 3;
        int areg = (reg - 0x20) & 0x07;
        auto& c = m_channels[ch];
        switch (areg) {
        case 0: c.volume = static_cast<int8_t>(val); break;
        case 1: c.feedback = val; break;
        case 2: break; // OUTPUT — read-only
        case 3: c.shiftRegister = (c.shiftRegister & 0xF00) | val; break;
        case 4: c.backup = val; break;
        case 5:
            c.control = val;
            c.timerEnabled = (val & 0x08) != 0;
            if (val & 0x10) { c.counter = c.backup; c.prescaleCounter = 0; }
            break;
        case 6: c.counter = val; break;
        case 7:
            c.other = val;
            c.shiftRegister = (c.shiftRegister & 0x0FF) | ((val & 0x0F) << 8);
            break;
        }
        return;
    }

    // Attenuation & stereo
    if (reg >= 0x40 && reg <= 0x44) return; // stored in m_regs
    if (reg == 0x50) return; // MSTEREO stored in m_regs

    // Interrupt reset — write 1s to CLEAR bits
    if (reg == 0x80) { m_interrupt_status &= ~val; return; } // INTRST

    // Interrupt set — write 1s to SET bits
    if (reg == 0x81) { m_interrupt_status |= val; return; }  // INTSET

    // SYSCTL1 at $FD87 — controls cart address and power
    if (reg == 0x87) {
        m_sysctl1 = val;
        // Bit 1: 0 = system reset (ignored in emulation for safety)
        // Bit 0: cart address strobe (1=HIGH, 0=LOW) — matches Handy
        if (m_cart)
            m_cart->cartAddressStrobe((val & 0x01) != 0);
        return;
    }

    // GPIO — IODIR at $FD8A, IODAT at $FD8B
    if (reg == 0x8A) { m_iodir = val; return; }
    if (reg == 0x8B) {
        m_iodat = val;
        updateCartLines();
        // Handy: AUDIN (bit 4) controls bank 1 write enable when IODIR bit 4 is output
        if (m_cart && (m_iodir & 0x10))
            m_cart->setWriteEnableBank1((val & 0x10) != 0);
        return;
    }

    // Serial
    if (reg == 0x8C) { m_serctl = val; return; }
    if (reg == 0x8D) { m_serdat = val; return; }

    // SDONEACK
    if (reg == 0x90) { return; }

    // CPU sleep
    if (reg == 0x91) { return; }

    // Display control
    if (reg == 0x92) { m_dispctl = val; return; } // DISPCTL
    if (reg == 0x93) { m_pbkup = val; return; }   // PBKUP
    if (reg == 0x94) { m_disp_addr = (m_disp_addr & 0xFF00) | val; return; }
    if (reg == 0x95) { m_disp_addr = (m_disp_addr & 0x00FF) | (uint16_t(val) << 8); return; }

    // Palette GREEN $FDA0–$FDAF
    if (reg >= 0xA0 && reg <= 0xAF) {
        m_palette_green[reg - 0xA0] = val;
        updatePaletteEntry(reg - 0xA0);
        return;
    }

    // Palette BLUERED $FDB0–$FDBF
    if (reg >= 0xB0 && reg <= 0xBF) {
        m_palette_br[reg - 0xB0] = val;
        updatePaletteEntry(reg - 0xB0);
        return;
    }

    // MAPCTL
    if (reg == 0xF9) { m_mapctl = val; return; }
}

// Propagate IODAT bit 1 → cart strobe, bit 0 → cart address data
void Mikey::updateCartLines()
{
    if (!m_cart) return;
    // Per Handy: IODAT bit 1 = cart address data
    // IODAT does NOT control strobe — strobe is only via SYSCTL1 bit 0
    m_cart->cartAddressData((m_iodat & 0x02) != 0);
}

// ════════════════════════════════════════════════════════════════════════
// Palette
// ════════════════════════════════════════════════════════════════════════

void Mikey::updatePaletteEntry(int idx)
{
    if (idx < 0 || idx >= 16) return;
    int g = m_palette_green[idx] & 0x0F;
    int b = (m_palette_br[idx] >> 4) & 0x0F;
    int r = m_palette_br[idx] & 0x0F;
    m_palette_rgb[idx] = 0xFF000000 | ((r * 17) << 16) | ((g * 17) << 8) | (b * 17);
}

// ════════════════════════════════════════════════════════════════════════
// Timer system
// ════════════════════════════════════════════════════════════════════════

int Mikey::timerPrescalerDivisor(uint8_t control)
{
    // Lynx timers: 16MHz master clock divided by (1 << (4 + LINKING))
    // LINKING=0 → 16 ticks (1µs), LINKING=1 → 32 (2µs), ..., LINKING=6 → 1024 (64µs)
    // LINKING=7 → linked mode (handled separately)
    static const int table[8] = { 16, 32, 64, 128, 256, 512, 1024, 0 };
    return table[control & 0x07];
}

// Lynx timer linking chains:
// Group A: Timer 0 → Timer 2 → Timer 4
// Group B: Timer 1 → Timer 3 → Timer 5 → Timer 7
static int timerLinkedNext(int idx)
{
    switch (idx) {
    case 0: return 2;
    case 2: return 4;
    case 1: return 3;
    case 3: return 5;
    case 5: return 7;
    default: return -1;
    }
}

void Mikey::tickTimer(int idx)
{
    auto& t = m_timers[idx];
    if (!(t.control & 0x08)) return; // not enabled (CTRL_A_COUNT)

    if ((t.control & 0x07) == 7) return; // linked mode — clocked by prev timer borrow

    int divisor = timerPrescalerDivisor(t.control);
    if (divisor <= 0) return; // linked mode (shouldn't reach here)
    t.prescaleCounter++;
    if (t.prescaleCounter < divisor) return;
    t.prescaleCounter = 0;

    decrementTimer(idx);
}

void Mikey::decrementTimer(int idx)
{
    auto& t = m_timers[idx];
    t.borrowFlag = false;

    if (t.counter == 0) {
        // Borrow: reload and fire
        t.counter = t.backup;
        t.borrowFlag = true;

        // Only set interrupt if this timer's IRQEN is set
        if (m_timer_irq_mask & (1 << idx))
            m_interrupt_status |= (1 << idx);

        // Timer 0 = display line
        if (idx == 0) {
            m_current_scanline++;
            if (m_current_scanline >= 105) {
                m_current_scanline = 0;
                m_frame_ready = true;
                m_last_frame = buildFrame();
            }
        }

        // Clock linked next timer in the chain
        int next = timerLinkedNext(idx);
        if (next >= 0 && next < 8) {
            auto& nt = m_timers[next];
            if ((nt.control & 0x07) == 7 && (nt.control & 0x08)) {
                decrementTimer(next);
            }
        }
    } else {
        t.counter--;
    }
}

// ════════════════════════════════════════════════════════════════════════
// Audio LFSR
// ════════════════════════════════════════════════════════════════════════

int Mikey::audioPrescalerDivisor(uint8_t control)
{
    // Same divisor table as system timers
    static const int table[8] = { 16, 32, 64, 128, 256, 512, 1024, 0 };
    return table[control & 0x07];
}

int Mikey::computeFeedback(const AudioChannel& ch) const
{
    static const int tapMap[8] = { 0, 2, 3, 4, 5, 7, 10, 11 };
    int fb = 0;
    for (int i = 0; i < 8; i++) {
        if (ch.feedback & (1 << i))
            fb ^= (ch.shiftRegister >> tapMap[i]) & 1;
    }
    return fb;
}

void Mikey::clockAudioChannel(int ch)
{
    auto& c = m_channels[ch];
    int newBit = computeFeedback(c);
    c.shiftRegister = ((c.shiftRegister << 1) | newBit) & 0xFFF;

    if (c.other & 0x80) { // integrate mode
        int sum = static_cast<int>(c.output) + static_cast<int>(c.volume);
        c.output = static_cast<int8_t>(std::clamp(sum, -128, 127));
    } else {
        c.output = (c.shiftRegister & 0x80) ? c.volume
                   : static_cast<int8_t>(-static_cast<int>(c.volume));
    }
}

// ════════════════════════════════════════════════════════════════════════
// Master tick (16 MHz)
// ════════════════════════════════════════════════════════════════════════

void Mikey::tick()
{
    for (int i = 0; i < 8; i++)
        tickTimer(i);

    for (int ch = 0; ch < 4; ch++) {
        auto& c = m_channels[ch];
        if (!c.timerEnabled) continue;
        if ((c.control & 0x07) == 7) continue;

        int divisor = audioPrescalerDivisor(c.control);
        c.prescaleCounter++;
        if (c.prescaleCounter < divisor) continue;
        c.prescaleCounter = 0;

        if (c.counter == 0) {
            c.counter = c.backup;
            clockAudioChannel(ch);

            int next = ch + 1;
            if (next < 4 && (m_channels[next].control & 0x07) == 7 && m_channels[next].timerEnabled) {
                if (m_channels[next].counter == 0) {
                    m_channels[next].counter = m_channels[next].backup;
                    clockAudioChannel(next);
                } else {
                    m_channels[next].counter--;
                }
            }
        } else {
            c.counter--;
        }
    }

    m_audio_tick_counter++;
    if (m_audio_tick_counter >= AUDIO_TICK_DIVIDER) {
        m_audio_tick_counter = 0;
        generateSample();
    }
}

void Mikey::generateSample()
{
    int left = 0, right = 0;
    for (int ch = 0; ch < 4; ch++) {
        const auto& c = m_channels[ch];
        if (c.other & 0x10) left  += c.output;
        if (c.other & 0x20) right += c.output;
    }

    int16_t leftSample  = static_cast<int16_t>(std::clamp(left  * 64, -32768, 32767));
    int16_t rightSample = static_cast<int16_t>(std::clamp(right * 64, -32768, 32767));

    static constexpr double INTERNAL_RATE = 16000000.0 / AUDIO_TICK_DIVIDER;
    m_audio_resample_acc += static_cast<double>(m_audio_sample_rate) / INTERNAL_RATE;

    while (m_audio_resample_acc >= 1.0) {
        m_audio_resample_acc -= 1.0;
        int nextW = (m_audio_buffer_write + 2) % AUDIO_BUFFER_SIZE;
        if (nextW != m_audio_buffer_read) {
            m_audio_buffer[m_audio_buffer_write]     = leftSample;
            m_audio_buffer[m_audio_buffer_write + 1]  = rightSample;
            m_audio_buffer_write = nextW;
        }
    }
}

int Mikey::readAudioBuffer(int16_t* dest, int maxSamples)
{
    int count = 0;
    while (count < maxSamples && m_audio_buffer_read != m_audio_buffer_write) {
        dest[count++] = m_audio_buffer[m_audio_buffer_read];
        m_audio_buffer_read = (m_audio_buffer_read + 1) % AUDIO_BUFFER_SIZE;
    }

    if (count > 0) {
        int16_t lastL = dest[count >= 2 ? count - 2 : 0];
        int16_t lastR = dest[count >= 2 ? count - 1 : 0];
        while (count < maxSamples - 1) {
            dest[count++] = lastL;
            dest[count++] = lastR;
        }
        if (count < maxSamples) dest[count++] = lastL;
    } else {
        std::memset(dest, 0, maxSamples * sizeof(int16_t));
        count = maxSamples;
    }
    return count;
}

// ════════════════════════════════════════════════════════════════════════
// Display rendering
// ════════════════════════════════════════════════════════════════════════

QImage Mikey::buildFrame() const
{
    QImage frame(160, 102, QImage::Format_RGB32);
    frame.fill(0xFF000000);

    if (!m_ram) return frame;

    uint16_t base = m_disp_addr;
    if (base == 0) base = 0xC000;

    bool flipH = (m_dispctl & 0x02) != 0;
    bool flipV = (m_dispctl & 0x04) != 0;

    for (int y = 0; y < 102; y++) {
        uint32_t *scanline = reinterpret_cast<uint32_t*>(frame.scanLine(flipV ? (101 - y) : y));
        int lineOffset = base + y * 80;

        for (int x = 0; x < 160; x++) {
            int byteIdx = lineOffset + x / 2;
            if (byteIdx >= 0x10000) continue;

            uint8_t byte = m_ram[byteIdx & 0xFFFF];
            uint8_t color = (x & 1) ? (byte & 0x0F) : ((byte >> 4) & 0x0F);

            int outX = flipH ? (159 - x) : x;
            scanline[outX] = m_palette_rgb[color];
        }
    }

    return frame;
}

QImage Mikey::renderFrame() const
{
    bool dmaEnabled = (m_dispctl & 0x01) != 0;
    if (!dmaEnabled && !m_last_frame.isNull())
        return m_last_frame;
    return buildFrame();
}

// ════════════════════════════════════════════════════════════════════════
// Serialization
// ════════════════════════════════════════════════════════════════════════

void Mikey::serialize(QDataStream &ds) const
{
    for (int i = 0; i < 8; i++) {
        ds << m_timers[i].backup << m_timers[i].control
           << m_timers[i].counter << m_timers[i].staticVal;
    }
    for (int i = 0; i < 4; i++) {
        const auto& c = m_channels[i];
        ds << static_cast<uint8_t>(c.volume) << c.feedback
           << static_cast<uint8_t>(c.output) << c.backup
           << c.control << c.counter << c.other << c.shiftRegister;
    }
    ds.writeRawData(reinterpret_cast<const char*>(m_palette_green), 16);
    ds.writeRawData(reinterpret_cast<const char*>(m_palette_br), 16);
    ds << m_interrupt_status;
    ds << m_dispctl << m_pbkup << m_mapctl;
    ds << static_cast<uint16_t>(m_current_scanline) << m_disp_addr;
    ds << m_iodir << m_iodat << m_sysctl1 << m_serctl << m_serdat;
}

void Mikey::deserialize(QDataStream &ds)
{
    for (int i = 0; i < 8; i++) {
        ds >> m_timers[i].backup >> m_timers[i].control
           >> m_timers[i].counter >> m_timers[i].staticVal;
    }
    for (int i = 0; i < 4; i++) {
        auto& c = m_channels[i];
        uint8_t vol, out;
        ds >> vol >> c.feedback >> out >> c.backup
           >> c.control >> c.counter >> c.other >> c.shiftRegister;
        c.volume = static_cast<int8_t>(vol);
        c.output = static_cast<int8_t>(out);
        c.timerEnabled = (c.control & 0x08) != 0;
    }
    ds.readRawData(reinterpret_cast<char*>(m_palette_green), 16);
    ds.readRawData(reinterpret_cast<char*>(m_palette_br), 16);
    for (int i = 0; i < 16; i++) updatePaletteEntry(i);
    ds >> m_interrupt_status;
    ds >> m_dispctl >> m_pbkup >> m_mapctl;
    uint16_t sl;
    ds >> sl >> m_disp_addr;
    m_current_scanline = sl;
    ds >> m_iodir >> m_iodat >> m_sysctl1 >> m_serctl >> m_serdat;
}
