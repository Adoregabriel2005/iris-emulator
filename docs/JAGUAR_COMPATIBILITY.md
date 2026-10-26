# Jaguar compatibility and test scope — v1.18.2

This page records observed results, not a promise that every level, control or sound effect works. Last updated: 26 October 2026.

## Games with targeted validation

| Game | Verified scope | Remaining scope |
|---|---|---|
| Doom | 3D demo; user-confirmed normal sound and speed; real WASAPI audio and 30 FPS presentation | Full campaign, all controls and all sound effects not certified |
| Wolfenstein 3D | DSP boot fix; 24 RAM states passed boot; four former failures also completed 3600 frames; real audio output | Complete gameplay and all stages not certified |
| Super Burnout | Menu text corrected; race rendering; real audio before/after Reset; PAL timing | Full races, steering and every menu combination not certified |
| Rayman | Opening regression and non-silent audio output in timed tests | Full game and all effects not certified |

All Jaguar games use 30 FPS presentation by default, while the console clock keeps normal NTSC/PAL speed. This setting does not establish compatibility for an otherwise failing game.

## Opening audit: all 139 local files

This audit used the repair build before the final audio/release packaging step, not a complete gameplay run of the published release. Each file ran in an isolated Qt process with BIOS off, default blitter, GPU/DSP enabled, sound off, no controller input, a 600-frame target and a 45-second process timeout. Variants, demos and homebrew count as separate files.

Results: **91 completed**, **44 rejected by the loader**, **4 timed out**, **0 unexpected process crashes**.

A completed test can still end on a black or solid-color image. A black capture is inconclusive: it can be a loading transition or a real rendering problem. Timeout does not mean the process crashed. None of these outcomes alone establishes full playability.

Audit executable SHA256: `bc9e83f8e987ea42c010976e6c61f33938b71b86682e2e1ec150d37241298352`.

[Download the text-only CSV](JAGUAR_OPENING_AUDIT.csv). No ROMs, screenshots, personal paths, saves or audio recordings are included.

| File | Opening result | Emulated frames | Last capture |
|---|---|---:|---|
| Alien vs Predator (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Alien vs Predator (Alpha).jag | Rejected by loader | 0 | No frame captured |
| Arkanna Demo (PD).jag | Rejected by loader | 0 | No frame captured |
| Assassin Demo, The - Release 2 (PD).jag | Rejected by loader | 0 | No frame captured |
| Assassin Demo, The Part 1 (1999) (PD).jag | Rejected by loader | 0 | No frame captured |
| Assassin Demo, The Part 1 (for BJL) (1999) (PD).jag | Rejected by loader | 0 | No frame captured |
| Asteroid (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| Atari Karts (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Attack of the Mutant Penguins (1996).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| BadCode0 by Badcoder (bin) (1999) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode0 by Badcoder (cof) (1999) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode1 by Badcoder (bin) (1999) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode1 by Badcoder (cof) (1999) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode2 by Badcoder (bin) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode2 by Badcoder (cof) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode3 by Badcoder (bin) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode3 by Badcoder (jag) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode4 (Metal) by Badcoder (bin) (2000) (PD) [a1].jag | Rejected by loader | 0 | No frame captured |
| BadCode4 (Metal) by Badcoder (bin) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode4 (Metal) by Badcoder (jag) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode4 (Metal) by Badcoder (rom) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode4 (Modified) by Badcoder (bin) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode4 (Modified) by Badcoder (jag) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode4 (Modified) by Badcoder (rom) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode4 by Badcoder (bin) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode4 by Badcoder (jag) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode4C by Badcoder (bin) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode4C by Badcoder (jag) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| BadCode4N by Badcoder (bin) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| BadCode4N by Badcoder (jag) (2000) (PD)badcde4n.jag | Completed opening test | 600 | Black; gameplay not validated |
| Brutal Sports Football (1994) (Telegames).jag | Rejected by loader | 0 | No frame captured |
| Bubsy - Fractured Furry Tails (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Cannon Fodder (1995) (Computer West).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Checkered Flag (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Chroma-Luma Color Pick (Mouse) by Matthias Domin (1996) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Chroma-Luma Color Pick by Matthias Domin (bin) (1996) (PD).jag | Rejected by loader | 0 | No frame captured |
| Chroma-Luma Color Pick by Matthias Domin (jag) (1996) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Chroma-Luma Color Pick by Matthias Domin (jag) (1996) (PD)[a1].jag | Completed opening test | 600 | Black; gameplay not validated |
| Club Drive (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| CRZ Demo (PD).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Cybermorph (1993).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Defender 2000 (1996).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| DEMO1 (bin) (PD).jag | Rejected by loader | 0 | No frame captured |
| DEMO1 (PD).jag | Timed out (45 s) | 600 | Solid color; gameplay not validated |
| DEMO1B (PD) [a1].jag | Rejected by loader | 0 | No frame captured |
| DEMO1B (PD).jag | Timed out (45 s) | 300 | Solid color; gameplay not validated |
| DEMO1C (PD).jag | Timed out (45 s) | 300 | Solid color; gameplay not validated |
| Doom - Evil Unleashed (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Double Dragon V (1995) (Williams).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Dragon - The Bruce Lee Story (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Drumpad by Robert Jurziga (2002) (PD).jag | Rejected by loader | 0 | No frame captured |
| Drumpad2 by Robert Jurziga (2002) (PD).jag | Rejected by loader | 0 | No frame captured |
| Evolution - Dino Dudes (1993).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Fever Pitch Soccer (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Fight For Your Life (1996) [a1].jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Fight For Your Life (1996) [a2].jag | Completed opening test | 600 | Black; gameplay not validated |
| Fight For Your Life (1996).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Flashback (1995) (U.S. Gold).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Flip Out (1995).jag | Completed opening test | 600 | Black; gameplay not validated |
| FORCE Design - Legion Force Jidai Intro Demo 0! (2001) (PD).jag | Rejected by loader | 0 | No frame captured |
| Gorf 2000 (PD).jag | Rejected by loader | 0 | No frame captured |
| Hover Strike (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Hubble Fade by Robert Jurziga (PD).jag | Rejected by loader | 0 | No frame captured |
| Hubble Nebula by Robert Jurziga (PD).jag | Rejected by loader | 0 | No frame captured |
| I-War (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| International Sensible Soccer (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Iron Soldier (1994) [a1].jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Iron Soldier (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| JagFest Demo (2001) (PD).jag | Rejected by loader | 0 | No frame captured |
| JagMania (Apr 18) (1998) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| JagMania (Aug 31) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| JagMania (Jan 06) (2001) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| JagMania (Jul 8) (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| JagMania (Sep 10) (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| JagMarble (1999) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| JagMarble (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| JagMarble (Jul 13) (1997) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server (Program Examples) - 27OBJ (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server (Program Examples) - 50HZ (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server (Program Examples) - 60HZ (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server (Program Examples) - INTRO (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server (Program Examples) - INTROMOD (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server (Program Examples) - SCALE (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server (Program Examples) - SCALE3 (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server 1.08 UPDATE - JAGOS (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Server 1.08 UPDATE - KEYB (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Jaguar Tetris (1995) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| JDC Demo V1 (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| JDC Demo V2 (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| JDC Demo V3 - E-JagFest Demo by Lars Hannig (2000) (PD).jag | Rejected by loader | 0 | No frame captured |
| JDC Example by Lars Hannig (PD).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Joypad-TeamTap Tester by Matthias Domin (2000) (PD) [a1].jag | Rejected by loader | 0 | No frame captured |
| Joypad-TeamTap Tester by Matthias Domin (2000) (PD) [a2].jag | Completed opening test | 600 | Black; gameplay not validated |
| Joypad-TeamTap Tester by Matthias Domin (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| JSSDemo (Jaguar Sound System) V1.0b (08.11.2002) (PD).jag | Rejected by loader | 0 | No frame captured |
| JSSDemoII (Jaguar Sound System) V1.0b (10.11.2002) (PD).jag | Rejected by loader | 0 | No frame captured |
| Kasumi Ninja (1994) [a1].jag | Completed opening test | 600 | Black; gameplay not validated |
| Kasumi Ninja (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Ladybug Demo (PD).jag | Timed out (45 s) | 300 | Solid color; gameplay not validated |
| Ladybug Demo (rom) (PD).jag | Rejected by loader | 0 | No frame captured |
| Mandelbrot Demo (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Memory Dump by Matthias Domin (1999) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Missile Command 3D (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Music Demo (2002) (ScatoLOGIC).jag | Rejected by loader | 0 | No frame captured |
| Native Demo (bin) (1997).jag | Rejected by loader | 0 | No frame captured |
| Native Demo (jag) (1997).jag | Completed opening test | 600 | Black; gameplay not validated |
| NBA Jam TE (1996).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Painter (1996) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Painter (bin) (1996) (PD).jag | Rejected by loader | 0 | No frame captured |
| PAULA Preview Demo 2 by Robert Jurziga (PD).jag | Rejected by loader | 0 | No frame captured |
| PAULA Preview Demo by Robert Jurziga (PD).jag | Rejected by loader | 0 | No frame captured |
| Phase Zero (2000) (PD) [a1].jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Phase Zero (2000) (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Pinball Fantasies (1995) (Computer West).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Pitfall - The Mayan Adventure (1995).jag | Completed opening test | 600 | Black; gameplay not validated |
| PlaySFX V1.0 by Robert Jurziga (2003) (PD).jag | Rejected by loader | 0 | No frame captured |
| Power Drive Rally (1995) (TWI).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| QSOUND Demo (PD).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Raiden (1994).jag | Completed opening test | 600 | Black; gameplay not validated |
| Rayman (1995) (UBI Soft).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Rayman Demo (1995) (UBI Soft).jag | Completed opening test | 600 | Black; gameplay not validated |
| Ruiner Pinball (1995).jag | Completed opening test | 600 | Black; gameplay not validated |
| SlamRacer Demo (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| SlamRacer Intro (PD).jag | Completed opening test | 600 | Black; gameplay not validated |
| Super Burnout (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Super Cross 3D (1995) [a1].jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Super Cross 3D (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Syndicate (1995) (Ocean).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Tempest 2000 (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Theme Park (1995) (Ocean).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Trevor McFur in the Crescent Galaxy (1993).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Troy Aikman NFL Football (1995) (Williams).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Ultra Vortek (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Ultra Vortek (Beta) (1995).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Val D'Isere Skiing & Snowboarding (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| White Men Can't Jump (1995).jag | Completed opening test | 600 | Black; gameplay not validated |
| Wolfenstein 3D (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Zool 2 (1994).jag | Completed opening test | 600 | Image produced; gameplay not validated |
| Zoop! (1996).jag | Completed opening test | 600 | Image produced; gameplay not validated |

## Unsupported / unresolved

- Jaguar CD is not implemented in this frontend (including Battlemorph and Blue Lightning CD).
- Brutal Sports Football was rejected before execution for the supplied image; earlier reports about audio desync do not describe this audit outcome.
- Other rejected formats and timed-out files are listed above. Do not classify them as supported based on a similar filename or another dump.
- Older reports mention graphical problems in Kasumi Ninja, Val d’Isère Skiing, Zool 2 and Bubsy, and performance drops in Iron Soldier. Those specific scenarios were not revalidated after v1.18.2 fixes.
