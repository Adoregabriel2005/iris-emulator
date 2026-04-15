#pragma once
#include <cstdint>

// ---------------------------------------------------------------------------
// Jaguar Compatibility Profile
//
// Each game gets a profile loaded at ROM boot based on jaguarMainROMCRC32.
// Flags control which emulation quirks are active for that specific game.
// Games not in the table get the DEFAULT profile, which preserves the
// behaviour that already works for 2D titles (Rayman, Tempest 2000, etc.).
//
// HOW TO ADD A GAME:
//   1. Run the game once — the CRC32 is printed in the debug log on load.
//   2. Add an entry to JAG_COMPAT_TABLE below with the appropriate flags.
// ---------------------------------------------------------------------------

struct JagCompatProfile
{
    uint32_t crc32;         // jaguarMainROMCRC32 — 0 = default/fallback

    // ── Object Processor ────────────────────────────────────────────────────
    // When true: OP stops processing the list on OBJECT_TYPE_GPU and waits
    // for the GPU to write OBF before continuing.  Required for correct 3D
    // rendering in games that use GPU objects as rendering barriers.
    // Breaks sprite-heavy 2D games (Rayman) if enabled without resume logic.
    bool op_stop_on_gpu_object;

    // ── DSP ─────────────────────────────────────────────────────────────────
    // Multiplier applied to DSP cycles per timeslice (1 = normal).
    // Doom/Wolfenstein need the DSP to run faster relative to the 68K to
    // avoid audio starvation and wall-cast stalls.
    int  dsp_cycle_mult;

    // ── Blitter ─────────────────────────────────────────────────────────────
    // Use the original blitter (blitter_blit) instead of BlitterMidsummer2.
    // The original handles GOURZ/SRCSHADE better for 3D polygon fills.
    bool use_original_blitter;

    // ── GPU ─────────────────────────────────────────────────────────────────
    // GPU cycle multiplier per timeslice (default 2, per JaguarExecuteNew).
    int  gpu_cycle_mult;

    // ── Display ─────────────────────────────────────────────────────────────
    // Force NTSC timing regardless of VMODE register (some 3D games set PAL
    // bits by mistake on NTSC hardware).
    bool force_ntsc;
};

// ---------------------------------------------------------------------------
// Default profile — safe for all 2D games
// ---------------------------------------------------------------------------
static constexpr JagCompatProfile JAG_PROFILE_DEFAULT = {
    /* crc32                */ 0,
    /* op_stop_on_gpu_object*/ false,
    /* dsp_cycle_mult       */ 1,
    /* use_original_blitter */ false,
    /* gpu_cycle_mult       */ 2,
    /* force_ntsc           */ false,
};

// ---------------------------------------------------------------------------
// Per-game table
// CRC32 values are printed by JaguarSystem on loadROM (check debug output).
// ---------------------------------------------------------------------------
static const JagCompatProfile JAG_COMPAT_TABLE[] =
{
    // ── Doom (Jaguar) ────────────────────────────────────────────────────────
    // Uses DSP for wall-casting, blitter for column rendering, GPU objects
    // as rendering barriers.  Needs original blitter + faster DSP.
    { 0x54A4D5EC, true,  2, true,  2, false },  // Doom (USA)

    // ── Wolfenstein 3D ───────────────────────────────────────────────────────
    { 0x86B1B0A2, true,  2, true,  2, false },  // Wolfenstein 3D (USA)

    // ── Checkered Flag ───────────────────────────────────────────────────────
    { 0x7AE20B9A, true,  1, true,  2, false },  // Checkered Flag (USA)

    // ── Club Drive ───────────────────────────────────────────────────────────
    { 0x3B2A3DEF, true,  1, true,  2, false },  // Club Drive (USA)

    // ── Cybermorph ───────────────────────────────────────────────────────────
    { 0x6E9A0F3C, false, 1, true,  2, false },  // Cybermorph (USA)

    // ── Iron Soldier ─────────────────────────────────────────────────────────
    { 0xA1B2C3D4, false, 1, true,  2, false },  // Iron Soldier (USA) — CRC TBD

    // ── Alien vs Predator ────────────────────────────────────────────────────
    { 0x5F6E7D8C, false, 1, false, 2, false },  // AvP (USA) — CRC TBD

    // ── Rayman ───────────────────────────────────────────────────────────────
    // 2D game — keep default, listed explicitly to document it's intentional
    { 0xFDF37F47, false, 1, false, 2, false },  // Rayman (USA)

    // ── Tempest 2000 ─────────────────────────────────────────────────────────
    { 0x84B3C2A1, false, 1, false, 2, false },  // Tempest 2000 (USA) — CRC TBD
};

static constexpr int JAG_COMPAT_TABLE_SIZE =
    static_cast<int>(sizeof(JAG_COMPAT_TABLE) / sizeof(JAG_COMPAT_TABLE[0]));

// ---------------------------------------------------------------------------
// Lookup — returns the profile for a given CRC32, or DEFAULT if not found.
// ---------------------------------------------------------------------------
inline const JagCompatProfile& jagFindProfile(uint32_t crc32)
{
    for (int i = 0; i < JAG_COMPAT_TABLE_SIZE; ++i)
        if (JAG_COMPAT_TABLE[i].crc32 == crc32)
            return JAG_COMPAT_TABLE[i];
    return JAG_PROFILE_DEFAULT;
}
