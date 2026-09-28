#include "settings.h"

// Only frontend-owned state belongs here. CPUs, memory, input and peripherals
// are provided by the Virtual Jaguar core, not replaced with no-op functions.
VJSettings vjs = {};
