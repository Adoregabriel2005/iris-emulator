#pragma once
#include <cstdint>

// ─────────────────────────────────────────────────────────────────────────────
// JaguarPatchDB.h
//
// Per-game patch table derived from BigPEmu scripts by Rich Whitehouse.
// Each entry maps a ROM CRC32 to a list of M68K breakpoint addresses and
// the action to take when that address is hit.
//
// CRC32 values are computed over the raw ROM data (jaguarMainROMCRC32 from
// the VJ core). The FNV1A64 hashes in the BigPEmu scripts are over the same
// data — we use CRC32 because the VJ core already computes it for us.
//
// Actions are simple enums — the JaguarSystem step() loop checks the M68K PC
// after every HandleNextEvent() and calls the appropriate handler.
// ─────────────────────────────────────────────────────────────────────────────

enum class JagPatchAction : uint8_t
{
    // DOOM
    Doom_MenuStall,         // 0x9CAA  — manual CPU stall when blitter hogs bus
    Doom_PatchGPUThrottle,  // patch GPU op throttle count each stall

    // Checkered Flag
    CF_UploadDrawCode,      // 0x802852 — inject GPU draw poly breakpoint
    CF_DrawSky,             // 0xD77B8  — sky draw, stall until scanout start
    CF_BufferFlip,          // 0xD675E  — clear native poly list
    CF_GameBegin,           // 0xD615C  — mark in-game
    CF_GameEnd,             // 0xD64E0  — mark out-of-game

    // AvP
    AvP_SetVidParams,       // 0x9022   — overwrite vbcount for better framerate

    // Cybermorph
    CM_UploadGPU,           // 0x8095E2 — track which GPU program is uploading
    CM_UploadGPUFinish,     // 0x80961A — inject GPU draw poly breakpoint
    CM_BgBlit,              // 0x809118 — clear poly list, set up native render
    CM_EndFrame,            // 0x80274A — clear current buffer

    // Wolf3D — no critical fix needed beyond DSP, placeholder
    // Iron Soldier — same
};

struct JagPatchEntry
{
    uint32_t       m68kPC;   // M68K address to intercept
    JagPatchAction action;
};

struct JagGamePatch
{
    uint32_t            crc32;
    const char*         name;
    const JagPatchEntry* entries;
    int                 count;
};

// ── DOOM ─────────────────────────────────────────────────────────────────────
// Hash: 0x75F2216BD4F54AC2 (FNV1A64) — CRC32 to be confirmed at runtime
// Key fix: menu stall (blitter suffocates CPU at menu)
static constexpr JagPatchEntry kDoomPatches[] = {
    { 0x00009CAA, JagPatchAction::Doom_MenuStall },
};

// ── Checkered Flag ────────────────────────────────────────────────────────────
// Hash: 0xF9DDA93597C567F7
static constexpr JagPatchEntry kCFPatches[] = {
    { 0x00802852, JagPatchAction::CF_UploadDrawCode },
    { 0x000D77B8, JagPatchAction::CF_DrawSky        },
    { 0x000D675E, JagPatchAction::CF_BufferFlip      },
    { 0x000D615C, JagPatchAction::CF_GameBegin       },
    { 0x000D64E0, JagPatchAction::CF_GameEnd         },
};

// ── Alien vs Predator ─────────────────────────────────────────────────────────
// Hash: AVP_SUPPORTED_HASH (in avp_common.h, not shown — use known CRC32)
// Key fix: vbcount override for better framerate (default 4 = 15Hz on NTSC)
static constexpr JagPatchEntry kAvPPatches[] = {
    { 0x00009022, JagPatchAction::AvP_SetVidParams },
};

// ── Cybermorph ────────────────────────────────────────────────────────────────
// Hash: 0x3F97A08E8550667C
static constexpr JagPatchEntry kCMPatches[] = {
    { 0x008095E2, JagPatchAction::CM_UploadGPU       },
    { 0x0080961A, JagPatchAction::CM_UploadGPUFinish },
    { 0x00809118, JagPatchAction::CM_BgBlit          },
    { 0x0080274A, JagPatchAction::CM_EndFrame        },
};

// ── Master patch table ────────────────────────────────────────────────────────
// CRC32 values will be matched against jaguarMainROMCRC32 at load time.
// Values marked TODO need to be confirmed by loading the ROM and logging the CRC.
// --- Rayman ---
// CRC32: 0xA9F8A00E (World)
static constexpr JagPatchEntry kRaymanPatches[] = {
    { 0x00000000, JagPatchAction::Doom_MenuStall }, // Dummy, substitua pelo hack real depois
};

static constexpr JagGamePatch kJagPatchDB[] = {
    { 0x00000000, "DOOM",           kDoomPatches, 1 }, // TODO: confirm CRC32
    { 0x00000000, "Checkered Flag", kCFPatches,   5 }, // TODO: confirm CRC32
    { 0x00000000, "AvP",            kAvPPatches,  1 }, // TODO: confirm CRC32
    { 0x00000000, "Cybermorph",     kCMPatches,   4 }, // TODO: confirm CRC32
    { 0xA9F8A00E, "Rayman",         kRaymanPatches, 1 }, // Dummy patch, troque pelo real depois
};
static constexpr int kJagPatchDBCount = 4;
