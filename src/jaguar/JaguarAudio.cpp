#include "JaguarAudio.h"
#include "dac.h"
#include "event.h"
#include "jerry.h"
#include "joystick.h"
#include <utility>

namespace {
std::vector<int16_t> samples;
constexpr double samplePeriod = 1000000.0 / 48000.0;
void sampleDAC()
{
    // Bound host buffering even when no audio device is available.
    if (samples.size() < 48000 * 2) {
        samples.push_back(audioEnabled ? static_cast<int16_t>(ltxd) : 0);
        samples.push_back(audioEnabled ? static_cast<int16_t>(rtxd) : 0);
    }
    SetCallbackTime(sampleDAC, samplePeriod);
}
}

std::vector<int16_t> JaguarTakeAudioSamples()
{
    std::vector<int16_t> result;
    result.swap(samples);
    return result;
}

void DACInit() { DACReset(); }
void DACReset()
{
    samples.clear();
    ltxd = rtxd = lrxd = rrxd = 0;
    sclk = 19;
    RemoveCallback(sampleDAC);
    SetCallbackTime(sampleDAC, samplePeriod);
}
void DACDone() { RemoveCallback(sampleDAC); samples.clear(); }
void DACPauseAudioThread(bool) {} // Host playback is controlled by JaguarSystem.

void DACWriteByte(uint32_t offset, uint8_t data, uint32_t who)
{
    if (offset == 0xF1A153) DACWriteWord(0xF1A152, data, who);
}
void DACWriteWord(uint32_t offset, uint16_t data, uint32_t)
{
    if (offset == 0xF1A14A) ltxd = data;
    else if (offset == 0xF1A14E) rtxd = data;
    else if (offset == 0xF1A152) {
        sclk = data & 0xFF;
        JERRYI2SInterruptTimer = -1;
        RemoveCallback(JERRYI2SCallback);
        JERRYI2SCallback();
    } else if (offset == 0xF1A156) smode = data;
}
uint8_t DACReadByte(uint32_t, uint32_t) { return 0xFF; }
uint16_t DACReadWord(uint32_t offset, uint32_t)
{
    if (offset == 0xF1A148 || offset == 0xF1A14C) return 0;
    if (offset == 0xF1A14A) return lrxd;
    if (offset == 0xF1A14E) return rrxd;
    return 0xFFFF;
}
