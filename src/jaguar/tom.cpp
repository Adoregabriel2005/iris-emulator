// TOM core variables for Jaguar (adaptado do Virtual Jaguar)
#include "tom.h"

// RAM principal do TOM (16K)
uint8_t tomRam8[0x4000];

// Largura e altura atuais do vídeo
uint32_t tomWidth = 0;
uint32_t tomHeight = 0;

// Outros registradores exportados
uint32_t tomTimerPrescaler = 0;
uint32_t tomTimerDivider = 0;
int32_t tomTimerCounter = 0;

// Buffer de vídeo e pitch
uint32_t* screenBuffer = nullptr;
uint32_t screenPitch = 0;
