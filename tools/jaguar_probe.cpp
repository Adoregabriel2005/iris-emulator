// ROM regression runner using the exact same core library as the Qt frontend.
// Usage: jaguar_probe game.j64 [frames=1800] [output.ppm] [--watch] [shot-every-frames]
#include "jaguar.h"
#include "file.h"
#include "settings.h"
#include "tom.h"
#include "log.h"
#include "modelsBIOS.h"
#include "JaguarAudio.h"
#include "m68000/m68kinterface.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <vector>

namespace {

const char * FileTypeName(uint32_t type)
{
    switch (type) {
    case JST_NONE: return "none";
    case JST_ROM: return "rom";
    case JST_ALPINE: return "alpine";
    case JST_ABS_TYPE1: return "abs1";
    case JST_ABS_TYPE2: return "abs2";
    case JST_JAGSERVER: return "jagserver";
    case JST_WTFOMGBBQ: return "wtfombbq";
    case JST_ELF32: return "elf32";
    default: return "?";
    }
}

void SaveFrame(const char* path, const uint32_t* video, unsigned pitch, unsigned height)
{
    const unsigned w = std::min(TOMGetVideoModeWidth(), pitch);
    const unsigned h = std::min(TOMGetVideoModeHeight() * ((TOMGetVP() & 1) ? 1u : 2u), height);
    FILE* f = std::fopen(path, "wb");
    if (!f) { std::fprintf(stderr, "Cannot write %s\n", path); return; }
    std::fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (unsigned y = 0; y < h; ++y) for (unsigned x = 0; x < w; ++x) {
        uint32_t pixel = video[y * pitch + x];
        unsigned char rgb[] = {static_cast<unsigned char>(pixel >> 24),
            static_cast<unsigned char>(pixel >> 16), static_cast<unsigned char>(pixel >> 8)};
        std::fwrite(rgb, 1, sizeof(rgb), f);
    }
    std::fclose(f);
}

void DumpFrameStats(const uint32_t* video, unsigned pitch, unsigned w, unsigned h)
{
    // A quick numeric summary: how much of the picture is actually drawn, how
    // many distinct colours, and how many scanlines are used. Text statistics
    // keep headless runs verifiable on machines without image display.
    std::map<uint32_t, unsigned> histogram;
    unsigned blankRows = 0;
    for (unsigned y = 0; y < h; ++y) {
        bool blank = true;
        for (unsigned x = 0; x < w; ++x) {
            const uint32_t p = video[y * pitch + x] & 0xFFFFFF;
            if (p) blank = false;
            ++histogram[p];
        }
        if (blank) ++blankRows;
    }
    unsigned nonBlack = 0;
    for (const auto& kv : histogram) if (kv.first) nonBlack += kv.second;
    std::printf("frame stats: %ux%u blankRows=%u distinctColours=%zu nonBlack=%u (%.1f%%)\n",
        w, h, blankRows, histogram.size(), nonBlack, 100.0 * nonBlack / (w * h));
    int shown = 0;
    for (const auto& kv : histogram) {
        if (!kv.first || shown >= 8) continue;
        std::printf("  %06X x%u\n", kv.first, kv.second);
        ++shown;
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::puts("Usage: jaguar_probe game.j64 [frames=1800|0] [output.ppm] [--watch|--oplog] [shot-every-frames]");
        return 2;
    }
    const int frames = argc > 2 ? std::atoi(argv[2]) : 1800;
    if (frames < 0 || frames > 36000) return 2;
    const char* output = argc > 3 ? argv[3] : "jaguar-frame.ppm";
    const bool watch = argc > 4 && std::strcmp(argv[4], "--watch") == 0;
    const bool oplog = argc > 4 && std::strcmp(argv[4], "--oplog") == 0;
    const int every = argc > 5 ? std::atoi(argv[5]) : 0;
    LogInit("jaguar-probe.log");
    vjs.hardwareTypeNTSC = true;
    vjs.DRAM_size = 0x200000;
    vjs.GPUEnabled = vjs.DSPEnabled = true;
    vjs.biosType = BT_M_SERIES;
    vjs.jaguarModel = JAG_M_SERIES;
    // Match the frontend defaults, without relying on host audio hardware.
    vjs.useFastBlitter = false;
    std::strcpy(vjs.EEPROMPath, ".");
    constexpr unsigned pitch = 2048, height = 512;
    std::vector<uint32_t> video(pitch * height, 0);
    JaguarInit();
    JaguarSetScreenBuffer(video.data());
    JaguarSetScreenPitch(pitch);
    if (!JaguarLoadFile(argv[1])) {
        std::fprintf(stderr, "Cannot load raw cartridge: %s\n", argv[1]);
        JaguarDone(); LogDone(); return 1;
    }
    // The type is only known after loading, so report it now: a wrong type is
    // what silently misplaces an image and starves the game of its entry point.
    {
        uint8_t* raw = nullptr;
        const uint32_t size = JaguarLoadROM(raw, argv[1]);
        const uint32_t type = size ? ParseFileType(raw, size) : JST_NONE;
        std::printf("file=%s size=%u type=%s\n", argv[1], size, FileTypeName(type));
        free(raw);
    }
    SelectBIOS(vjs.biosType);
    SET32(jaguarMainRAM, 0, vjs.DRAM_size);
    JaguarReset();
    std::printf("CRC32=%08X entry=%08X frames=%d\n", jaguarMainROMCRC32, jaguarRunAddress, frames);
    extern bool frameDone;
    extern uint32_t jaguarROMSize;
    extern uint32_t irisWatchPCLow, irisWatchPCHigh, irisWatchCount;
    extern int op_start_log;
    if (watch) {
        irisWatchPCLow = jaguarRunAddress;
        irisWatchPCHigh = 0x800000 + jaguarROMSize;
        irisWatchCount = 1;
    }
    if (frames == 0) {
        std::printf("Load-only run: nothing executed.\n");
        JaguarDone();
        LogDone();
        return 0;
    }
    for (int frame = 1; frame <= frames; ++frame) {
        // --oplog dumps the object list of the final frame only, which is what
        // tells us how wide the game thinks its objects are.
        op_start_log = (oplog && frame == frames) ? 1 : 0;
        JaguarExecuteNew();
        JaguarTakeAudioSamples();
        if (!frameDone) {
            std::fprintf(stderr, "Frame %d incomplete, PC=%08X\n", frame, m68k_get_reg(nullptr, M68K_REG_PC));
            JaguarDone(); LogDone(); return 1;
        }
        if (frame % 300 == 0) std::printf("frame=%d PC=%08X\n", frame, m68k_get_reg(nullptr, M68K_REG_PC));
        if (every > 0 && frame % every == 0) {
            char name[512];
            std::snprintf(name, sizeof(name), "%s.%04d.ppm", output, frame);
            SaveFrame(name, video.data(), pitch, height);
        }
    }
    const unsigned w = std::min(TOMGetVideoModeWidth(), pitch);
    const unsigned h = std::min(TOMGetVideoModeHeight() * ((TOMGetVP() & 1) ? 1u : 2u), height);
    SaveFrame(output, video.data(), pitch, h);
    DumpFrameStats(video.data(), pitch, w, h);
    std::printf("Saved %s (%ux%u). Completing frames does not prove game compatibility.\n", output, w, h);
    JaguarDone();
    LogDone();
}
