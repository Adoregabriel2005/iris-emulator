#include "jaguar.h"
#include "memory.h"
#include "settings.h"
#include "event.h"
#include "tom.h"
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
    testRiscExecution();
    testBlitter();
    testScanlineBounds();
    testPixelWidthReplication();
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
