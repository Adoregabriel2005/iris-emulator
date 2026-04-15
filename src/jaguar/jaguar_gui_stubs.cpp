// jaguar_gui_stubs.cpp
// Replaces VJ RX interactive GUI functions with silent no-ops.
// The linker picks these over the definitions in jaguar.cpp because
// this file is compiled directly into the Iris target.

#include <cstdio>
#include <cstdint>
#include "m68000/m68kinterface.h"

static bool throttle_log(unsigned int address, unsigned int value)
{
    static uint32_t counter = 0;
    static unsigned int lastAddr = 0xFFFFFFFFu;
    static unsigned int lastVal  = 0xFFFFFFFFu;
    ++counter;
    if (address != lastAddr || value != lastVal) {
        lastAddr = address; lastVal = value; return true;
    }
    return (counter & 0x1FFu) == 0;
}

bool m68k_read_exception_vector(unsigned int address, char *text)
{
    static uint32_t n = 0;
    if ((++n & 0x3Fu) == 1)
        printf("Jaguar: exception at $%06X: %s\n", address, text ? text : "?");
    return false;
}

bool m68k_write_unknown_alert(unsigned int address, char *bits, unsigned int value)
{
    if (throttle_log(address, value))
        printf("Jaguar: unknown write (%s) $%X at $%06X\n", bits ? bits : "?", value, address);
    return false;
}

bool m68k_write_cartridge_alert(unsigned int address, char *bits, unsigned int value)
{
    if (throttle_log(address, value))
        printf("Jaguar: ROM write (%s) $%X at $%06X\n", bits ? bits : "?", value, address);
    return false;
}
