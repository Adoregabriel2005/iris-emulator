// Stubs completos para o core Jaguar - versão standalone
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "settings.h"
#include "JaguarSafeWrite.h"

// ============================================================
// Memória Jaguar (6MB RAM)
// ============================================================
uint8_t jagMemSpace[0x600000];

// Joypad buttons - arrays alocados estáticamente
static unsigned char joypad0ButtonsArray[21];
static unsigned char joypad1ButtonsArray[21];
extern "C" unsigned char* joypad0Buttons = joypad0ButtonsArray;
extern "C" unsigned char* joypad1Buttons = joypad1ButtonsArray;

bool jaguarCartInserted = false;
uint32_t jaguarROMSize = 0;

// Universal cart header
uint8_t universalCartHeader[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

// Variáveis de ROM
uint32_t jaguarMainROMCRC32 = 0;
uint32_t jaguarRunAddress = 0;

// TOM variables - necessárias para JaguarDebugWindow e tom_utils
uint8_t tomRam8[0x10000];
uint32_t tomWidth = 320;
uint32_t tomHeight = 240;
uint32_t screenPitch = 320;
uint32_t* screenBuffer = nullptr;

// Settings global
VJSettings vjs = {};

// ============================================================
// Áudio stubs
// ============================================================
void DACPauseAudioThread(bool) {}
void DACDone(void) {}

// ============================================================
// Log stubs
// ============================================================
int LogInit(const char*) { return 0; }
void LogDone(void) {}
void WriteLog(const char*, ...) {}

// ============================================================
// ZIP/CRC stubs
// ============================================================
struct ZipFileEntry { int dummy; };
bool GetZIPHeader(FILE*, ZipFileEntry&) { return false; }
int UncompressFileFromZIP(FILE*, ZipFileEntry, unsigned char*) { return 0; }
int crc32_calcCheckSum(unsigned char*, unsigned int) { return 0; }

// ============================================================
// M68K memory functions
// ============================================================
extern "C" {
    unsigned char m68k_read_memory_8(unsigned int address) {
        if (address >= 0x800000 && address < 0xE00000) {
            return jagMemSpace[address - 0x800000];
        }
        return 0xFF;
    }
    
    unsigned short m68k_read_memory_16(unsigned int address) {
        if (address >= 0x800000 && address < 0xE00000) {
            uint32_t offset = address - 0x800000;
            return (jagMemSpace[offset] << 8) | jagMemSpace[offset + 1];
        }
        return 0xFFFF;
    }
    
    unsigned int m68k_read_memory_32(unsigned int address) {
        if (address >= 0x800000 && address < 0xE00000) {
            uint32_t offset = address - 0x800000;
            return (jagMemSpace[offset] << 24) | (jagMemSpace[offset + 1] << 16) |
                   (jagMemSpace[offset + 2] << 8) | jagMemSpace[offset + 3];
        }
        return 0xFFFFFFFF;
    }
    
    void m68k_write_memory_8(unsigned int address, unsigned int value) {
        if (address >= 0x800000 && address < 0xE00000) {
            jagMemSpace[address - 0x800000] = value & 0xFF;
        }
    }
    
    void m68k_write_memory_16(unsigned int address, unsigned int value) {
        if (address >= 0x800000 && address < 0xE00000) {
            uint32_t offset = address - 0x800000;
            jagMemSpace[offset] = (value >> 8) & 0xFF;
            jagMemSpace[offset + 1] = value & 0xFF;
        }
    }
    
    void m68k_write_memory_32(unsigned int address, unsigned int value) {
        if (address >= 0x800000 && address < 0xE00000) {
            uint32_t offset = address - 0x800000;
            jagMemSpace[offset] = (value >> 24) & 0xFF;
            jagMemSpace[offset + 1] = (value >> 16) & 0xFF;
            jagMemSpace[offset + 2] = (value >> 8) & 0xFF;
            jagMemSpace[offset + 3] = value & 0xFF;
        }
    }
    
    int irq_ack_handler(void) { return 0; }
    void M68KInstructionHook(void) {}
    int m68k_get_reg(void* context, int regnum) { return 0; }
    void m68k_set_reg(void* context, int regnum, int value) {}
    void m68k_set_irq(int) {}
    void m68k_end_timeslice(void) {}
    void m68k_pulse_reset(void) {}
}

// ============================================================
// Jaguar core stubs - todas as funções que o JaguarSystem chama
// ============================================================
void JaguarSetScreenBuffer(uint32_t* buffer) {}
void JaguarSetScreenPitch(uint32_t pitch) {}

void JaguarInit(void) {
    // Inicializa memória
    memset(jagMemSpace, 0xFF, 0x600000);
}

void JaguarReset(void) {
    memset(jagMemSpace, 0xFF, 0x600000);
}

void JaguarDone(void) {}

void JaguarExecuteNew(void) {
    // Stub - não executa nada
}

uint16_t JaguarReadWord(uint32_t addr, uint32_t) {
    if (addr >= 0x800000 && addr < 0xE00000) {
        uint32_t offset = addr - 0x800000;
        return (jagMemSpace[offset] << 8) | jagMemSpace[offset + 1];
    }
    return 0xFFFF;
}

uint32_t JaguarReadLong(uint32_t addr, uint32_t) {
    if (addr >= 0x800000 && addr < 0xE00000) {
        uint32_t offset = addr - 0x800000;
        return (jagMemSpace[offset] << 24) | (jagMemSpace[offset + 1] << 16) |
               (jagMemSpace[offset + 2] << 8) | jagMemSpace[offset + 3];
    }
    return 0xFFFFFFFF;
}

void JaguarWriteWord(uint32_t addr, uint16_t value, uint32_t) {
    if (addr >= 0x800000 && addr < 0xE00000) {
        uint32_t offset = addr - 0x800000;
        jagMemSpace[offset] = (value >> 8) & 0xFF;
        jagMemSpace[offset + 1] = value & 0xFF;
    }
}

// ============================================================
// DSP/DAC stubs
// ============================================================
void DSPExec(int) {}
bool DSPIsRunning(void) { return false; }
void DSPExecP2(int) {}
void SetCallbackTime(void (*)(void), double, int) {}
void RemoveCallback(void (*)(void)) {}
double GetTimeToNextEvent(int) { return 0.0; }
void HandleNextEvent(int) {}
void JERRYI2SCallback(void) {}
int JERRYI2SInterruptTimer = 0;
unsigned short ltxd = 0;
unsigned short lrxd = 0;
unsigned short rtxd = 0;
unsigned short rrxd = 0;
unsigned char sclk = 0;
unsigned int smode = 0;
const char* whoName[] = { nullptr };

// ============================================================
// Debug vars
// ============================================================
int start_logging = 0;
bool interactiveMode = false;
bool blitterSingleStep = false;
bool bssGo = false;
bool bssHeld = false;
bool GUIKeyHeld = false;
bool keyHeld1 = false, keyHeld2 = false, keyHeld3 = false;
bool iLeft = false, iRight = false, iToggle = false;
int objectPtr = 0;
bool startMemLog = false;
bool doDSPDis = false, doGPUDis = false;
int gpu_start_log = 0;
int op_start_log = 0;
int blit_start_log = 0;
int effect_start = 0;
int effect_start2 = 0, effect_start3 = 0, effect_start4 = 0, effect_start5 = 0, effect_start6 = 0;

// ============================================================
// Joystick stub functions
// ============================================================
void JoystickInit(void) {}
void JoystickExec(void) {}
void JoystickReset(void) {}
void JoystickDone(void) {}
uint16_t JoystickReadWord(uint32_t) { return 0xFFFF; }
void JoystickWriteWord(uint32_t, uint16_t) {}

// ============================================================
// M68K disassembler symbols
// ============================================================
typedef void (*cpuop_func)(int);
static void IllegalOpcodeHandler(int) {}
extern "C" cpuop_func IllegalOpcode = IllegalOpcodeHandler;
extern "C" cpuop_func cpuFunctionTable[65536] = { nullptr };
