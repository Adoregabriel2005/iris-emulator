// dsp_dac_stubs.cpp - Stubs for DSP and DAC functions
#include <cstdint>

// DSP execution stubs
void DSPExec(int) {}
bool DSPIsRunning(void) { return false; }
void DSPExecP2(int) {}

// Event/callback system
void SetCallbackTime(void (*)(void), double, int) {}
void RemoveCallback(void (*)(void)) {}
double GetTimeToNextEvent(int) { return 0.0; }
void HandleNextEvent(int) {}

// JERRY I2S
void JERRYI2SCallback(void) {}
int JERRYI2SInterruptTimer = 0;

// Serial communication stubs
unsigned short ltxd = 0;
unsigned short lrxd = 0;
unsigned short rtxd = 0;
unsigned short rrxd = 0;
unsigned char sclk = 0;
unsigned int smode = 0;

// Who name (debug)
const char* whoName[] = { nullptr };
