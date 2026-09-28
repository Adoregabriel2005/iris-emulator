#pragma once
#include <cstdint>
#include <vector>

// Called by the emulation thread after a frame. The SDL device only consumes
// these samples; host audio callbacks must never execute the DSP.
std::vector<int16_t> JaguarTakeAudioSamples();
