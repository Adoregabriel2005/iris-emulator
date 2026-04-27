
#include <cstdint>
#include <cstdio>

extern "C" {
void DACPauseAudioThread(bool) {}
void DACDone(void) {}

void m68k_set_reg(void* context, int regnum, int value) {}

int LogInit(const char*) { return 0; }
void LogDone(void) {}

bool jaguarCartInserted = false;
extern unsigned char* joypad0Buttons;
extern unsigned char* joypad1Buttons;
extern uint8_t* jagMemSpace;
extern uint8_t* universalCartHeader;
extern unsigned int jaguarROMSize;

void WriteLog(const char*, ...) {}

// Dummy struct para ZipFileEntry
struct ZipFileEntry { int dummy; };
bool GetZIPHeader(FILE*, ZipFileEntry&) { return false; }
int UncompressFileFromZIP(FILE*, ZipFileEntry, unsigned char*) { return 0; }

int crc32_calcCheckSum(unsigned char*, unsigned int) { return 0; }
}
