// jaguar_gui_stubs.cpp
// Replaces VJ's interactive GUI functions (QMessageBox popups) with
// silent no-op versions suitable for headless use inside Iris Emulator.
//
// The linker will pick these definitions instead of the ones in jaguar.cpp
// because this file is compiled as part of the Iris target directly.
//
// Functions stubbed:
//   m68k_read_exception_vector  -> log + return false (don't halt)
//   m68k_write_unknown_alert    -> log + return false
//   m68k_write_cartridge_alert  -> log + return false

#include <cstdio>
#include <cstdint>
#include "m68000/m68kinterface.h"

static bool should_log_throttled(unsigned int address, unsigned int value)
{
    static uint32_t counter = 0;
    static unsigned int lastAddr = 0xFFFFFFFFu;
    static unsigned int lastVal  = 0xFFFFFFFFu;

    ++counter;
    // Sempre loga quando muda padrão de escrita; para repetição, loga 1 a cada 512
    if (address != lastAddr || value != lastVal) {
        lastAddr = address;
        lastVal  = value;
        return true;
    }
    return (counter & 0x1FFu) == 0;
}

bool m68k_read_exception_vector(unsigned int address, char *text)
{
    static uint32_t exCounter = 0;
    ++exCounter;
    if ((exCounter & 0x3Fu) == 1) {
        printf("Jaguar [stub]: exception vector at $%06X: %s\n", address, text ? text : "(null)");
    }
    return false;
}

bool m68k_write_unknown_alert(unsigned int address, char *bits, unsigned int value)
{
    if (should_log_throttled(address, value)) {
        printf("Jaguar [stub]: unknown write (%s) $%X at $%06X\n", bits ? bits : "?", value, address);
    }
    return false;
}

bool m68k_write_cartridge_alert(unsigned int address, char *bits, unsigned int value)
{
    if (should_log_throttled(address, value)) {
        printf("Jaguar [stub]: cartridge write (%s) $%X at $%06X\n", bits ? bits : "?", value, address);
    }
    return false;
}
