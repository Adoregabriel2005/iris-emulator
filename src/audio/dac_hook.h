// dac_hook.h — exposes a post-processing hook for the VJ DAC audio callback.
// Include this in JaguarSystem.cpp BEFORE including dac.h.
// The hook is called after SDLSoundCallback fills the buffer.
#pragma once
#include <SDL.h>
#include <cstdint>

// Set by VSTHost::installSDLCallbackWrapper() before DACInit is called.
// SDLSoundCallback checks this and calls it after filling the buffer.
typedef void (*DACPostProcFn)(int16_t* samples, int numStereoFrames);
extern DACPostProcFn g_dacPostProc;
