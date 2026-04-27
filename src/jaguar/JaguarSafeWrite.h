#pragma once
#include <stdint.h>
extern uint8_t jagMemSpace[];
// Escrita segura na memória Jaguar: ignora acessos inválidos
inline void JaguarSafeWrite(uint32_t address, uint8_t value) {
    if (address >= 0x800000 && address < 0xE00000) {
        jagMemSpace[address - 0x800000] = value;
    }
}
