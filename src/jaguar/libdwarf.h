#pragma once
// libdwarf.h stub for Iris — DWARF debug info not used in headless mode.
typedef void Dwarf_Debug;
typedef void Dwarf_Die;
typedef void Dwarf_Error;
typedef unsigned long Dwarf_Unsigned;
typedef unsigned long Dwarf_Off;
typedef unsigned long Dwarf_Addr;
typedef int Dwarf_Bool;
typedef void* Dwarf_Ptr;
#define DW_DLV_OK 0
#define DW_DLV_NO_ENTRY -1
#define DW_DLV_ERROR 1
static inline int dwarf_init(int fd, int access, void* eh, void* ea, Dwarf_Debug* dbg, Dwarf_Error* err) { return DW_DLV_NO_ENTRY; }
static inline int dwarf_finish(Dwarf_Debug dbg, Dwarf_Error* err) { return DW_DLV_OK; }
