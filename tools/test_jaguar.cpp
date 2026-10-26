#include "jaguar.h"
#include "memory.h"
#include "settings.h"
#include "event.h"
#include "tom.h"
#include "op.h"
#include "m68000/m68kinterface.h"
#include "JaguarAudio.h"
#include "gpu.h"
#include "dsp.h"
#include "blitter.h"
#include "file.h"
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

static void check(bool condition, const char* message)
{
    if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}
static int fired = 0;
static void testStereoAudio()
{
    JaguarReset();
    JaguarTakeAudioSamples();
    JaguarWriteWord(0xF14000, 0x0100); // Jaguar hardware sound enable.
    JaguarWriteWord(0xF1A14A, 1234);
    JaguarWriteWord(0xF1A14E, uint16_t(-2345));
    JaguarExecuteNew();
    auto pcm = JaguarTakeAudioSamples();
    check(pcm.size() >= 1500 && pcm.size() % 2 == 0, "48 kHz stereo samples continue at native console rate");
    for (size_t i = 0; i < pcm.size(); i += 2)
        check(pcm[i] == 1234 && pcm[i + 1] == -2345, "signed left/right PCM channel order");
    JaguarWriteWord(0xF14000, 0);
    JaguarExecuteNew();
    pcm = JaguarTakeAudioSamples();
    check(!pcm.empty() && std::all_of(pcm.begin(), pcm.end(), [](int16_t v) { return v == 0; }),
        "hardware mute preserves timing and outputs silence");
    JaguarReset();
    check(JaguarTakeAudioSamples().empty(), "reset removes queued samples from the previous session");
}
static void testBlankingObjectWriteback()
{
    for (bool ntsc : {true, false}) {
        vjs.hardwareTypeNTSC = ntsc;
        JaguarReset();
        const unsigned lastVisible = ntsc ? 510 : 578;
        const unsigned lateLine = ntsc ? 522 : 622;
        TOMWriteWord(0xF00046, 25);
        TOMWriteWord(0xF00048, 0xFFFF);
        TOMWriteWord(0xF00028, (3 << 9) | 6 | 0x80);
        TOMWriteWord(0xF00020, 0);
        TOMWriteWord(0xF00022, 7); // OLP = $70000, low word first.
        auto put = [](uint32_t address, uint64_t phrase) {
            JaguarWriteLong(address, phrase >> 32);
            JaguarWriteLong(address + 4, uint32_t(phrase));
        };
        const uint64_t bitmap = (uint64_t(0x71000) << 40) | (uint64_t(0x70020) << 21)
            | (uint64_t(8) << 14) | (lastVisible << 3);
        const uint64_t scaled = (uint64_t(0x71000) << 40) | (uint64_t(0x70040) << 21)
            | (uint64_t(8) << 14) | (lastVisible << 3) | 1;
        const uint64_t format = (uint64_t(1) << 28) | (1 << 18) | (1 << 15) | (4 << 12);
        put(0x70000, bitmap); put(0x70008, format);
        put(0x70020, scaled); put(0x70028, format); put(0x70030, 0x2020);
        put(0x70040, 2); // GPU object, followed by STOP.
        put(0x70048, 4);
        TOMExecHalfline(lastVisible, true);
        check(OPLoadPhrase(0x70000) != bitmap && OPLoadPhrase(0x70020) != scaled,
            "last visible line must still advance fixed and scaled sprites");
        // The VBL handler rebuilds these sprites for the next field.
        put(0x70000, bitmap); put(0x70020, scaled); put(0x70030, 0x2020);
        GPUSetIRQLine(GPUIRQ_OBJECT, 0);
        TOMExecHalfline(lateLine, true);
        check(OPLoadPhrase(0x70000) == bitmap && OPLoadPhrase(0x70020) == scaled
            && OPLoadPhrase(0x70030) == 0x2020,
            "blanking must not consume rebuilt sprite rows or scaling remainder");
        check((GPUReadLong(0xF02114) & (1u << (6 + GPUIRQ_OBJECT))) != 0,
            "blanking must still deliver GPU-object interrupts");
    }
}
static void first() { fired = 1; }
static void second() { fired = 2; }
extern void tom_render_16bpp_cry_scanline(uint32_t*);
extern void tom_render_16bpp_rgb_scanline(uint32_t*);
extern void tom_render_16bpp_cry_rgb_mix_scanline(uint32_t*);
extern void tom_render_24bpp_scanline(uint32_t*);

static void testScanlineBounds()
{
    // A game may change its video registers between modes. An offscreen HDB1
    // must not underflow the remaining uint16_t width and overwrite later rows.
    constexpr uint32_t sentinel = 0xDEADBEEF;
    tomWidth = 326;
    SET16(tomRam8, 0x28, 3 << 9); // PWIDTH=4
    SET16(tomRam8, 0x38, 0x7FE);  // display starts beyond the visible viewport
    using Renderer = void (*)(uint32_t*);
    for (Renderer render : {tom_render_16bpp_cry_scanline, tom_render_16bpp_rgb_scanline,
            tom_render_16bpp_cry_rgb_mix_scanline, tom_render_24bpp_scanline}) {
        std::vector<uint32_t> row(70000, sentinel);
        render(row.data());
        check(std::all_of(row.begin() + tomWidth, row.end(),
            [](uint32_t p) { return p == sentinel; }), "offscreen HDB1 must not overwrite following rows");
    }
    // Ordinary visible RGB pixels and the left border must retain their colors.
    const unsigned left = vjs.hardwareTypeNTSC ? 188 : 204;
    SET16(tomRam8, 0x38, left + 4 * 8);
    tomRam8[0x2A] = 0x22; tomRam8[0x2B] = 0x11; tomRam8[0x2D] = 0x33;
    SET16(tomRam8, 0x1800, 0xF800); // full red in Jaguar RGB16
    std::vector<uint32_t> row(tomWidth + 1, sentinel);
    tom_render_16bpp_rgb_scanline(row.data());
    check(row[0] == 0x112233FF && row[7] == 0x112233FF, "normal left border preserved");
    check(row[8] == 0xF80000FF, "normal RGB pixel preserved");
    check(row[tomWidth] == sentinel, "normal scanline boundary preserved");
}

static void testRiscExecution()
{
    // MOVEQ #19,r0; JR T,*; NOP (delay slot). Both RISC processors must
    // execute once per emulated time slice even when host sound is disabled.
    const uint16_t code[] = {uint16_t((35 << 10) | (19 << 5)),
        uint16_t((53 << 10) | (31 << 5)), uint16_t(57 << 10)};
    for (int i = 0; i < 3; ++i) {
        GPUWriteWord(0xF03000 + i * 2, code[i]);
        DSPWriteWord(0xF1B000 + i * 2, code[i]);
    }
    GPUWriteLong(0xF02110, 0xF03000);
    DSPWriteLong(0xF1A110, 0xF1B000);
    GPUWriteLong(0xF02114, 1);
    DSPWriteLong(0xF1A114, 1);
    vjs.audioEnabled = false;
    JaguarExecuteNew();
    check(gpu_reg_bank_0[0] == 19, "GPU executes with muted audio");
    check(dsp_reg_bank_0[0] == 19, "DSP executes with muted audio");
    GPUWriteLong(0xF02114, 0);
    DSPWriteLong(0xF1A114, 0);
}

static void testDspFlagPipeline()
{
    // Wolf3D's ISR clears IMASK immediately before JUMP (r3). The jump
    // operand was read from bank 0, while the resumed code uses bank 1.
    for (unsigned shape = 0; shape < 3; ++shape) {
        DSPReset();
        const uint32_t base = 0xF1B000, good = base + 0x100, bad = base + 0x200;
        auto put = [](uint32_t address, uint16_t opcode) { DSPWriteWord(address, opcode); };
        const uint16_t storeFlags = (47 << 10) | (1 << 5); // STORE r0,(r1)
        const uint16_t jump = (52 << 10) | (3 << 5);      // JUMP T,(r3)
        const uint16_t marker = (35 << 10) | (9 << 5) | 6;
        const uint16_t nop = 57 << 10;
        const uint16_t loop = (53 << 10) | (31 << 5);
        put(base, shape == 1 ? jump : storeFlags);
        put(base + 2, shape == 0 ? jump : (shape == 1 ? storeFlags : marker));
        put(base + 4, shape == 0 ? marker : (shape == 1 ? nop : jump));
        put(base + 6, nop);
        put(good, (35 << 10) | (19 << 5) | 5);
        put(good + 2, loop); put(good + 4, nop);
        put(bad, (35 << 10) | (7 << 5) | 5);
        put(bad + 2, loop); put(bad + 4, nop);
        dsp_reg_bank_0[0] = 0x4210; // REGPAGE, clear CPU latch, keep CPU IRQ enabled
        dsp_reg_bank_0[1] = 0xF1A100;
        dsp_reg_bank_0[3] = good;
        dsp_reg_bank_0[31] = 0xF1CFE0;
        dsp_reg_bank_1[3] = shape == 2 ? good : bad;
        DSPWriteLong(0xF1A100, 0x4010);
        DSPWriteLong(0xF1A110, good);
        DSPSetIRQLine(DSPIRQ_CPU, ASSERT_LINE);
        check(DSPReadLong(0xF1A100) & 8, "DSP interrupt enters masked bank 0");
        DSPWriteLong(0xF1A114, 1);
        DSPExec(20);
        if (dsp_reg_bank_1[5] != 19) {
            std::fprintf(stderr, "DSP flag pipeline shape=%u marker=%u PC=%08X\n",
                shape, dsp_reg_bank_1[5], DSPReadLong(0xF1A110));
            check(false, "DSP interrupt returns through the pre-store register bank");
        }
        if (shape == 0)
            check(dsp_reg_bank_1[6] == 9, "branch delay slot sees retired bank switch");
        if (shape == 2)
            check(dsp_reg_bank_0[6] == 9 && dsp_reg_bank_1[6] == 0,
                "instruction following FLAGS store writes the old bank");
        DSPWriteLong(0xF1A114, 0);
    }
    DSPReset();
    const uint32_t base = 0xF1B000, target = base + 0x100;
    DSPWriteWord(base, (47 << 10) | (1 << 5));
    DSPWriteWord(base + 2, (52 << 10) | (3 << 5));
    DSPWriteWord(base + 4, 57 << 10);
    dsp_reg_bank_0[0] = 0x4010; // leave the CPU interrupt latched
    dsp_reg_bank_0[1] = 0xF1A100;
    dsp_reg_bank_0[3] = target;
    dsp_reg_bank_0[31] = 0xF1CFE0;
    DSPWriteLong(0xF1A100, 0x4010);
    DSPSetIRQLine(DSPIRQ_CPU, ASSERT_LINE);
    DSPWriteLong(0xF1A114, 1);
    DSPExec(1); // FLAGS store
    DSPExec(1); // return branch, including its delay slot
    check(DSPReadLong(0xF1A110) == target && dsp_reg_bank_0[31] == 0xF1CFDC,
        "pending DSP IRQ cannot preempt return branch or its delay slot");
    DSPExec(1);
    check(dsp_reg_bank_0[31] == 0xF1CFD8, "pending DSP IRQ is serviced after return");
    DSPWriteLong(0xF1A114, 0);
    DSPReset();
    dsp_reg_bank_0[31] = 0xF1CFE0;
    DSPWriteLong(0xF1A100, 0x10);
    DSPSetIRQLine(DSPIRQ_CPU, ASSERT_LINE);
    DSPWriteLong(0xF1A100, 0x18);
    check(DSPReadLong(0xF1A100) & 8, "writing one preserves the hardware DSP interrupt mask");
    DSPWriteLong(0xF1A100, 0x10);
    check(!(DSPReadLong(0xF1A100) & 8), "writing zero clears the DSP interrupt mask");
    DSPWriteLong(0xF1A100, 0x18);
    check(!(DSPReadLong(0xF1A100) & 8), "software cannot set the DSP interrupt mask");
    DSPReset();
}

static void testBlitter()
{
    // Copy 16-bit pixels in pixel and phrase modes, through both existing
    // implementations. Keep unaligned base semantics used by Rayman intact.
    for (bool fast : {false, true}) for (bool phrase : {false, true}) {
        vjs.useFastBlitter = fast;
        BlitterReset();
        constexpr uint32_t source = 0x10000, dest = 0x20000;
        for (unsigned i = 0; i < 8; ++i) {
            JaguarWriteWord(source + i * 2, 0x1200 + i);
            JaguarWriteWord(dest + i * 2, 0xEEEE);
        }
        JaguarWriteWord(dest + 16, 0xCAFE);
        const uint32_t flags = 0x1820 | (phrase ? 0 : 0x10000); // 8-wide, 16bpp
        BlitterWriteLong(0xF02200, dest + 3);
        BlitterWriteLong(0xF02204, flags);
        BlitterWriteLong(0xF02224, source + 5);
        BlitterWriteLong(0xF02228, flags);
        BlitterWriteLong(0xF0223C, 0x00010008);
        BlitterWriteLong(0xF02238, 0x01800009); // LFU source, SRCEN, DSTEN
        for (unsigned i = 0; i < 8; ++i)
            check(JaguarReadWord(dest + i * 2) == 0x1200 + i, "blitter copy / phrase-aligned base");
        check(JaguarReadWord(dest + 16) == 0xCAFE, "blitter preserves destination boundary");
    }
}

static void testBlitterDepth()
{
    // A polygon must read the depth buffer even when colour reads are off.
    // Exercise all comparison masks and both destination address generators.
    for (bool fast : {false, true}) for (bool a2 : {false, true})
    for (bool readColour : {false, true}) for (bool phrase : {false, true})
    for (bool background : {false, true})
    for (unsigned mode = 0; mode < 8; ++mode) for (unsigned z : {99u, 100u, 101u}) {
        vjs.useFastBlitter = fast;
        BlitterReset();
        constexpr uint32_t dest = 0x20000;
        for (unsigned x = 0; x < 4; ++x) {
            JaguarWriteWord(dest + x * 2, 0x1234);
            JaguarWriteWord(dest + 8 + x * 2, 100);
        }
        JaguarWriteWord(dest + 16, 0xCAFE);
        const uint32_t flags = 0x1860 | (phrase ? 0 : 0x10000);
        BlitterWriteLong(0xF02200, dest);
        BlitterWriteLong(0xF02204, flags);
        BlitterWriteLong(0xF02224, dest);
        BlitterWriteLong(0xF02228, flags);
        BlitterWriteLong(0xF02240, 0xABCDABCD);
        BlitterWriteLong(0xF02244, 0xABCDABCD);
        BlitterWriteLong(0xF02248, 0x12341234);
        BlitterWriteLong(0xF0224C, 0x12341234);
        BlitterWriteLong(0xF02258, (z << 16) | z);
        BlitterWriteLong(0xF0225C, (z << 16) | z);
        BlitterWriteLong(0xF0223C, 0x00010004);
        BlitterWriteLong(0xF02238, 0x01800030 | (mode << 18) | (a2 ? 0x800 : 0) |
            (readColour ? 8 : 0) | (background ? 0x10000000 : 0));
        const bool inhibited = (z < 100 && (mode & 1)) ||
            (z == 100 && (mode & 2)) || (z > 100 && (mode & 4));
        for (unsigned x = 0; x < 4; ++x) {
            if (JaguarReadWord(dest + x * 2) != (inhibited ? 0x1234 : 0xABCD) ||
                JaguarReadWord(dest + 8 + x * 2) != (inhibited ? 100 : z)) {
                std::fprintf(stderr, "depth case: fast=%d a2=%d mode=%u z=%u readColour=%d phrase=%d background=%d pixel=%u\n",
                    fast, a2, mode, z, readColour, phrase, background, x);
                check(false, "independent depth read, comparison and write");
            }
        }
        check(JaguarReadWord(dest + 16) == 0xCAFE, "depth writes preserve adjacent phrase");
    }
}

static void testLoader()
{
    // 128 KB is the smallest size the core accepts as a physical cartridge.
    std::vector<uint8_t> rom(131072, 0xFF);
    rom[0x404] = 0; rom[0x405] = 0x80; rom[0x406] = 0x20; rom[0x407] = 0;
    rom[0x2000] = 0x70; rom[0x2001] = 42;
    rom[0x2002] = 0x60; rom[0x2003] = 0xFE;
    char name[] = "jaguar-synthetic-test.j64";
    FILE* f = std::fopen(name, "wb");
    check(f != nullptr, "create synthetic cartridge");
    check(std::fwrite(rom.data(), 1, rom.size(), f) == rom.size(), "write synthetic cartridge");
    std::fclose(f);
    JaguarWriteLong(0x1000, 0x12345678);
    check(JaguarLoadFile(name), "load synthetic cartridge");
    std::remove(name);
    check(JaguarReadLong(0x1000) == 0x12345678, "loading ROM cannot overwrite RAM");
    check(JaguarReadWord(0x802000) == 0x702A, "ROM loaded at real bus address");
    check(jaguarRunAddress == 0x802000 && jaguarMainROMCRC32 != 0, "ROM entry point and CRC");
}

static void testPixelWidthReplication()
{
    // Doom draws its 3D viewport as a 160 pixel wide 16bpp object and its
    // status bar as a 320 pixel wide 8bpp object, then relies on PWIDTH = 8 to
    // stretch the viewport across the screen. The line buffer always holds the
    // virtual screen, so a wide PWIDTH has to duplicate pixels; leaving the
    // remainder of the row stale is what made 3D games flicker between a
    // half-width and a full-width picture.
    const unsigned left = vjs.hardwareTypeNTSC ? 188 : 204;
    // Self-calibrate the two colours the renderer produces, so the test does
    // not have to hardcode the RGB16 lookup table.
    tomWidth = 4;
    SET16(tomRam8, 0x38, left);
    SET16(tomRam8, 0x1800, 0xF800);
    SET16(tomRam8, 0x1802, 0x001F);
    uint32_t probe[8] = {};
    tom_render_16bpp_rgb_scanline(probe);
    const uint32_t colourA = probe[0], colourB = probe[1];
    check(colourA != colourB, "replication test needs two distinct colours");

    for (unsigned pwidth : {4u, 8u}) {
        tomWidth = 1304 / pwidth;
        SET16(tomRam8, 0x28, ((pwidth - 1) << 9) | 6); // 16bpp RGB without BGEN
        SET16(tomRam8, 0x38, left);
        for (unsigned x = 0; x < 720; ++x)
            SET16(tomRam8, 0x1800 + x * 2, (x & 1) ? 0x001F : 0xF800);
        std::vector<uint32_t> row(340, 0xDEADBEEFu);
        TOMRenderScanline(row.data());
        bool intact = true, pattern = true;
        for (unsigned x = 0; x < 326; ++x) {
            if (row[x] == 0xDEADBEEFu) intact = false;
            // The line buffer alternates A, B, A, B... A wide PWIDTH must turn
            // that into A, A, B, B... across the full width of the row.
            const unsigned source = pwidth == 4 ? x : x / (pwidth / 4);
            if (row[x] != (source & 1 ? colourB : colourA)) pattern = false;
        }
        check(intact, "PWIDTH fills the whole rendered row");
        char message[80];
        std::snprintf(message, sizeof(message),
            "PWIDTH %u duplicates pixels as expected", pwidth);
        check(pattern, message);
    }
}

int main()
{
    InitializeEventList();
    SetCallbackTime(first, 1);
    SetCallbackTime(second, 2);
    RemoveCallback(first);
    check(GetTimeToNextEvent() == 2, "ignore removed earliest event");
    HandleNextEvent();
    check(fired == 2, "execute remaining event");
    check(std::isinf(GetTimeToNextEvent()), "empty event queue");
    HandleNextEvent();
    check(fired == 2, "empty queue cannot replay a callback");

    vjs.hardwareTypeNTSC = true;
    vjs.DRAM_size = 0x200000;
    vjs.GPUEnabled = vjs.DSPEnabled = true;
    JaguarInit();
    std::vector<uint32_t> video(2048 * 512 + 64, 0);
    std::fill(video.end() - 64, video.end(), 0xCAFE1234);
    JaguarSetScreenBuffer(video.data());
    JaguarSetScreenPitch(2048);
    // Tiny synthetic 68000 cartridge: MOVEQ #42,D0; BRA.S *.
    jagMemSpace[0x802000] = 0x70; jagMemSpace[0x802001] = 42;
    jagMemSpace[0x802002] = 0x60; jagMemSpace[0x802003] = 0xFE;
    extern uint32_t jaguarRunAddress;
    jaguarRunAddress = 0x802000;
    SET32(jaguarMainRAM, 0, vjs.DRAM_size);
    JaguarReset();
    check(m68k_get_reg(nullptr, M68K_REG_PC) == 0x802000, "68000 reset vector");
    JaguarExecuteNew();
    check(m68k_get_reg(nullptr, M68K_REG_D0) == 42, "real 68000 instruction execution");
    auto audio = JaguarTakeAudioSamples();
    check(!audio.empty(), "DAC advances without host audio device");
    JaguarWriteLong(0x1000, 0x12345678);
    check(JaguarReadLong(0x1000) == 0x12345678, "shared RAM big-endian access");
    check(m68k_read_memory_32(0x1000) == 0x12345678, "68000 sees peripheral memory writes");
    JaguarReset();
    check(JaguarReadWord(0x802000) == 0x702A, "reset preserves cartridge");
    testLoader();
    testStereoAudio();
    testRiscExecution();
    testDspFlagPipeline();
    testBlitter();
    testBlitterDepth();
    testScanlineBounds();
    testPixelWidthReplication();
    testBlankingObjectWriteback();
    // Repeated mode changes exercise complete frames, including PAL interlace.
    extern bool frameDone;
    for (bool ntsc : {true, false}) for (bool interlaced : {false, true}) {
        vjs.hardwareTypeNTSC = ntsc;
        JaguarReset();
        const uint16_t vp = TOMGetVP();
        TOMWriteWord(0xF0003E, interlaced ? (vp & ~1) : (vp | 1));
        for (unsigned pwidth = 1; pwidth <= 8; ++pwidth) {
            TOMWriteWord(0xF00028, ((pwidth - 1) << 9) | 6);
            check(TOMGetVideoModeWidth() == 1304 / pwidth, "viewport matches TOM pixel width");
            for (int i = 0; i < 3; ++i) {
                JaguarExecuteNew();
                check(frameDone, "complete frame after video mode change");
                auto frameAudio = JaguarTakeAudioSamples();
                check(!frameAudio.empty(), "audio continues across video modes");
            }
        }
    }
    check(std::all_of(video.end() - 64, video.end(),
        [](uint32_t p) { return p == 0xCAFE1234; }), "PAL interlace framebuffer boundary");
    JaguarDone();
    std::puts("PASS: Jaguar events, 68000/GPU/DSP, ROM, memory, blitters, scanlines, NTSC/PAL/interlace and audio");
}
