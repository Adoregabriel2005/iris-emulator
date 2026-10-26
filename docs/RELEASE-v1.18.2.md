# Íris Emulator v1.18.2

## Jaguar

- 30 FPS presentation by default while preserving normal NTSC/PAL console speed.
- On-screen FPS, console frequency and speed percentage, including fullscreen.
- Real 48 kHz stereo output, configured audio buffering and recovery after underruns.
- Reset now reopens the audio device. The output-device selector saves stable device identifiers.
- DSP register-bank retirement and interrupt-return fix for intermittent Wolfenstein 3D boot hangs.
- Object-processor fix for clipped Super Burnout menu/HUD text while preserving late GPU interrupts.
- Blitter depth-read correction, scanline boundary checks and pixel-width replication.
- Independent Jaguar keyboard bindings: arrows, Z/X/C = A/B/C, Enter = Pause, O = Option, digits = keypad.

## Validation

Three CTest targets passed (Jaguar core, input and timing). Core tests cover signed stereo PCM, hardware mute/reset, register-bank timing, Z comparisons, scanline boundaries and NTSC/PAL object processing. Timing tests verify 30 FPS presentation without halving console speed and bounded recovery after short host stalls.

Before release version metadata was added, real WASAPI playback tests passed for Doom (1800 emulated frames), Wolfenstein (900), Super Burnout with Reset (900) and PAL Super Burnout (300), with no underruns, queue errors/resets or saturated samples in those final measurements. The user confirmed normal sound and speed in Doom. Wolfenstein boot was also checked across 24 initial RAM states; Super Burnout menu text was visually compared before and after the fix.

The versioned release executable is rebuilt from the same implementation and receives an additional audio/timing smoke test. The Windows package contains the executable and runtime dependencies, without ROMs, personal configuration, EEPROM saves or logs.

## Limits

This is not a claim of full Jaguar compatibility. A full-ROM-set opening audit of 139 local files completed 600 frames for 91 files, rejected 44 formats and timed out on four. Those results do not certify complete gameplay, every level or audio for the whole catalogue. Jaguar CD remains unsupported by this frontend.

Use the new v1.18.2 executable; older test builds may have a silent audio driver or different timing controls. Start with the system-default audio device and 128 ms audio latency. Existing per-user volume settings are retained.
