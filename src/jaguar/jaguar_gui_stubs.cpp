// jaguar_gui_stubs.cpp
// VJ core defines m68k_read_exception_vector, m68k_write_unknown_alert and
// m68k_write_cartridge_alert directly in jaguar.cpp using Qt dialogs.
// Those Qt dialog calls now use QString::asprintf (Qt6-compatible) and are
// overridden at link time by the definitions in jaguar.cpp itself.
// This file is intentionally empty — kept in the build for documentation.
