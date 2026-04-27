// jaguar_file_stub.cpp
// Replaces JAGUAR REFERENCE 1/src/file.cpp (which depends on libelf/gelf).
// Implements only the functions the VJ core actually calls at runtime.

#include "file.h"
#include "log.h"
#include "memory.h"
#include "settings.h"
#include "unzip.h"
#include "universalhdr.h"
#include "crc32.h"

#include "JaguarSafeWrite.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

extern uint8_t  jagMemSpace[];
extern uint32_t jaguarROMSize;
extern uint32_t jaguarRunAddress;
extern uint32_t jaguarMainROMCRC32;
extern bool     jaguarCartInserted;

// ---------------------------------------------------------------------------
// Load raw binary from disk
// ---------------------------------------------------------------------------
static uint32_t loadRaw(uint8_t *& buf, const char * path)
{
    FILE * f = fopen(path, "rb");
    if (!f) { WriteLog("FILE: cannot open '%s'\n", path); return 0; }
    fseek(f, 0, SEEK_END);
    uint32_t size = (uint32_t)ftell(f);
    rewind(f);
    buf = (uint8_t *)malloc(size);
    if (!buf) { fclose(f); return 0; }
    fread(buf, 1, size, f);
    fclose(f);
    return size;
}

// ---------------------------------------------------------------------------
// Try to load from ZIP, fall back to raw
// ---------------------------------------------------------------------------
uint32_t JaguarLoadROM(uint8_t *& rom, char * path)
{
    // Check for ZIP extension
    const char * ext = strrchr(path, '.');
    if (ext && (_stricmp(ext, ".zip") == 0))
    {
        FILE * f = fopen(path, "rb");
        if (f)
        {
            ZipFileEntry entry;
            if (GetZIPHeader(f, entry) && entry.uncompressedSize > 0)
            {
                rom = (uint8_t *)malloc(entry.uncompressedSize);
                if (rom)
                {
                    int result = UncompressFileFromZIP(f, entry, rom);
                    fclose(f);
                    if (result == 0)
                        return entry.uncompressedSize;
                    free(rom); rom = nullptr;
                }
            }
            fclose(f);
        }
    }
    return loadRaw(rom, path);
}

// ---------------------------------------------------------------------------
// Main load entry point called by JaguarSystem
// ---------------------------------------------------------------------------
bool JaguarLoadFile(char * path)
{
    uint8_t * rom  = nullptr;
    uint32_t  size = JaguarLoadROM(rom, path);
    if (!size || !rom)
    {
        WriteLog("FILE: JaguarLoadFile failed for '%s'\n", path);
        return false;
    }

    // Skip 8 KB universal header if present
    uint32_t offset = 0;
    if (size > 8192 && memcmp(rom, universalCartHeader, 8) == 0)
        offset = 8192;

    uint8_t * data  = rom + offset;
    uint32_t  dsize = size - offset;

    // Run address from bytes 0x404–0x407 of the ROM data
    jaguarRunAddress = (dsize >= 0x408)
        ? ((uint32_t)data[0x404] << 24 | (uint32_t)data[0x405] << 16
         | (uint32_t)data[0x406] <<  8 | (uint32_t)data[0x407])
        : 0x802000;

    // Copy into Jaguar address space at $800000 (max 6 MB)

    // Limpa a área de ROM
    for (uint32_t i = 0; i < 0x600000; ++i)
        JaguarSafeWrite(0x800000 + i, 0xFF);
    // Copia a ROM, byte a byte, usando JaguarSafeWrite
    uint32_t copySize = (dsize > 0x600000) ? 0x600000 : dsize;
    for (uint32_t i = 0; i < copySize; ++i)
        JaguarSafeWrite(0x800000 + i, data[i]);
    jaguarROMSize = copySize;

    jaguarMainROMCRC32 = (uint32_t)crc32_calcCheckSum(data, dsize);
    jaguarCartInserted = true;

    WriteLog("FILE: loaded '%s'  size=%u  runAddr=$%08X  CRC32=$%08X\n",
             path, dsize, jaguarRunAddress, jaguarMainROMCRC32);

    free(rom);
    return true;
}

bool AlpineLoadFile(char *)
{
    WriteLog("FILE: AlpineLoadFile not supported in Iris build\n");
    return false;
}

bool DebuggerLoadFile(char *)
{
    WriteLog("FILE: DebuggerLoadFile not supported in Iris build\n");
    return false;
}

uint32_t GetFileFromZIP(const char *, FileType, uint8_t *&) { return 0; }
uint32_t GetFileDBIdentityFromZIP(const char *)             { return 0; }
bool     FindFileInZIPWithCRC32(const char *, uint32_t)     { return false; }
uint32_t ParseFileType(uint8_t *, uint32_t)                 { return JST_ROM; }
bool     HasUniversalHeader(uint8_t * rom, uint32_t size)
{
    return (size > 8192 && memcmp(rom, universalCartHeader, 8) == 0);
}
