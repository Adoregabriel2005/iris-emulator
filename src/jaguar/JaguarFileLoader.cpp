// JaguarFileLoader.cpp
// Jaguar software loading for the Iris build.
//
// The reference core (JAGUAR REFERENCE 1/src/file.cpp) cannot be compiled as-is
// because its ELF branch needs libelf/libdwarf. This file reimplements every
// format that the emulator actually runs, keeping the reference detection rules
// and load addresses:
//
//   JST_ROM         raw cartridge, $800000, run address from ROM+0x404
//   JST_ALPINE      headerless image, $802000 (the 8 KB gap at $800000 is $FF)
//   JST_ELF32       32-bit big-endian 68k ELF, PT_LOAD segments
//   JST_ABS_TYPE1   ABS/COFF type 1
//   JST_ABS_TYPE2   ABS/COFF type 2
//   JST_JAGSERVER   load address at 0x22, run address at 0x2A
//   JST_WTFOMGBBQ   little-endian load/start address at 0x1C
//
// Every write goes through JaguarCopyImage(), which refuses to run past the
// address space instead of overflowing jagMemSpace the way the reference does
// (it carries its own "!!! FIX !!!" warning about exactly that).

#include "file.h"
#include "log.h"
#include "memory.h"
#include "settings.h"
#include "eeprom.h"
#ifdef IRIS_JAGUAR_ZIP
#include "unzip.h"
#include "zlib.h"
#endif
#include "universalhdr.h"
#include "crc32.h"
#include "jaguar.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>

// Elf32_Ehdr field offsets (big-endian), so that this file does not need elf.h.
static const uint32_t EI_CLASS_OFF = 0x04, ELFCLASS32_VALUE = 1;
static const uint32_t ET_EXEC_OFF = 0x10, EM_68K_OFF = 0x12;
static const uint32_t PT_LOAD = 1;

extern uint8_t  jagMemSpace[];
extern uint32_t jaguarROMSize;
extern uint32_t jaguarRunAddress;
extern uint32_t jaguarMainROMCRC32;
extern bool     jaguarCartInserted;
extern uint8_t *jaguarMainRAM;

// Registers live at $DFFF00 and above; loaded images must stay below that.
static const uint32_t kImageTop = 0xDFFF00;
static const uint32_t kMemSpaceSize = 0xF20000;

// Copies a loaded image into Jaguar address space, clipping to the address
// space instead of writing past it. Returns the number of bytes accepted.
static uint32_t JaguarCopyImage(uint32_t address, const uint8_t * source, uint32_t length)
{
	if (address >= kImageTop)
	{
		WriteLog("FILE: image at $%08X is outside loadable memory, ignoring %u bytes\n", address, length);
		return 0;
	}
	if (length > kImageTop - address)
	{
		WriteLog("FILE: image at $%08X overruns loadable memory, clipping %u to %u bytes\n",
			address, length, kImageTop - address);
		length = kImageTop - address;
	}
	if (length)
	{
		memcpy(jagMemSpace + address, source, length);
	}
	return length;
}

static void JaguarFillImage(uint32_t address, uint8_t value, uint32_t length)
{
	if (address >= kImageTop)
	{
		return;
	}
	if (length > kImageTop - address)
	{
		length = kImageTop - address;
	}
	memset(jagMemSpace + address, value, length);
}

// ---------------------------------------------------------------------------
// Load raw binary (or a ZIP member) from disk
// ---------------------------------------------------------------------------
static uint32_t loadRaw(uint8_t *& buf, const char * path)
{
	FILE * f = fopen(path, "rb");
	if (!f) { WriteLog("FILE: cannot open '%s'\n", path); return 0; }
	fseek(f, 0, SEEK_END);
	const long length = ftell(f);
	if (length <= 0 || length > 0x602000) { fclose(f); return 0; }
	uint32_t size = static_cast<uint32_t>(length);
	rewind(f);
	buf = (uint8_t *)malloc(size);
	if (!buf) { fclose(f); return 0; }
	if (fread(buf, 1, size, f) != size) { free(buf); buf = nullptr; fclose(f); return 0; }
	fclose(f);
	return size;
}

uint32_t JaguarLoadROM(uint8_t *& rom, char * path)
{
	char * ext = strrchr(path, '.');
	if (!ext)
	{
		WriteLog("FILE: '%s' has no extension, refusing to load it\n", path);
		return 0;
	}

	if (strcmp(ext, ".zip") == 0 || strcmp(ext, ".ZIP") == 0)
	{
#ifdef IRIS_JAGUAR_ZIP
		FILE * f = fopen(path, "rb");
		if (f)
		{
			ZipFileEntry entry;
			if (GetZIPHeader(f, entry) && entry.uncompressedSize > 0 && entry.uncompressedSize <= 0x602000)
			{
				rom = (uint8_t *)malloc(entry.uncompressedSize);
				if (rom)
				{
					const int result = UncompressFileFromZIP(f, entry, rom);
					fclose(f);
					if (result == 0)
					{
						return entry.uncompressedSize;
					}
					free(rom); rom = nullptr; return 0;
				}
			}
			fclose(f);
		}
#endif
		// Never execute an unsupported/invalid ZIP as a raw cartridge.
		WriteLog("FILE: cannot read '%s' as a ZIP cartridge\n", path);
		return 0;
	}

#ifdef IRIS_JAGUAR_ZIP
	// Gzipped images are transparent, exactly like in the reference core.
	gzFile gz = gzopen(path, "rb");
	if (gz)
	{
		gzseek(gz, 0, SEEK_END);
		const long gzSize = gztell(gz);
		gzseek(gz, 0, SEEK_SET);
		if (gzSize > 0 && gzSize <= 0x602000)
		{
			rom = (uint8_t *)malloc(gzSize);
			if (rom)
			{
				const int got = gzread(gz, rom, (unsigned)gzSize);
				gzclose(gz);
				if (got == (int)gzSize)
				{
					return (uint32_t)gzSize;
				}
				free(rom); rom = nullptr; return 0;
			}
		}
		gzclose(gz);
	}
#endif

	return loadRaw(rom, path);
}

// ---------------------------------------------------------------------------
// File type detection, following the reference rules
// ---------------------------------------------------------------------------
uint32_t ParseFileType(uint8_t * buffer, uint32_t size)
{
	if (!buffer || size < 0x30)
	{
		return JST_NONE;
	}

	// ELF 32 bits, big-endian 68k executable
	if (buffer[EI_CLASS_OFF] == ELFCLASS32_VALUE)
	{
		if (buffer[0] == 0x7F && buffer[1] == 'E' && buffer[2] == 'L' && buffer[3] == 'F' &&
			GET16(buffer, ET_EXEC_OFF) == 2 &&					// ET_EXEC
			GET16(buffer, EM_68K_OFF) == 4)					// EM_68K
		{
			return JST_ELF32;
		}
	}

	// ABS/COFF type 1
	if (buffer[0] == 0x60 && buffer[1] == 0x1B)
		return JST_ABS_TYPE1;

	// ABS/COFF type 2
	if (buffer[0] == 0x01 && buffer[1] == 0x50)
		return JST_ABS_TYPE2;

	// Jag Server & other old shite
	if (buffer[0] == 0x60 && buffer[1] == 0x1A)
	{
		if (buffer[0x1C] == 'J' && buffer[0x1D] == 'A' && buffer[0x1E] == 'G')
			return JST_JAGSERVER;
		return JST_WTFOMGBBQ;
	}

	// Divisible by 1 MB: a regular ROM (2 MB / 4 MB) or a 128 KB cart.
	if ((size % 1048576) == 0 || size == 131072)
		return JST_ROM;

	// 8 KB short of a whole number of MB: an Alpine image.
	if (((size + 8192) % 1048576) == 0)
		return JST_ALPINE;

	return JST_NONE;
}

bool HasUniversalHeader(uint8_t * rom, uint32_t romSize)
{
	if (!rom || romSize < 8192)
		return false;

	for (uint32_t i = 0; i < 8192; i++)
		if (rom[i] != universalCartHeader[i])
			return false;

	return true;
}

// ---------------------------------------------------------------------------
// ELF32 loading (program headers only; no libelf)
// ---------------------------------------------------------------------------
static bool loadELF32(const uint8_t * buffer, uint32_t size)
{
	if (size < 0x34)
	{
		WriteLog("FILE: ELF image is too small\n");
		return false;
	}

	const uint32_t entry = GET32(buffer, 0x18);
	const uint32_t phoff = GET32(buffer, 0x1C);
	const uint16_t phentsize = GET16(buffer, 0x2A);
	const uint16_t phnum = GET16(buffer, 0x2C);
	if (phnum == 0 || phentsize < 0x20)
	{
		WriteLog("FILE: ELF image has no usable program headers\n");
		return false;
	}
	if ((uint64_t)phoff + (uint64_t)phentsize * phnum > size)
	{
		WriteLog("FILE: ELF program headers do not fit in the file\n");
		return false;
	}

	uint32_t loaded = 0;
	for (uint16_t i = 0; i < phnum; i++)
	{
		const uint8_t * ph = buffer + phoff + i * phentsize;
		const uint32_t type = GET32(ph, 0x00);
		if (type != PT_LOAD)
			continue;
		const uint32_t offset = GET32(ph, 0x04);
		const uint32_t vaddr = GET32(ph, 0x08);
		const uint32_t filesz = GET32(ph, 0x10);
		if (filesz == 0)
			continue;
		if ((uint64_t)offset + filesz > size)
		{
			WriteLog("FILE: ELF segment %u runs past the end of the file\n", i);
			return false;
		}
		jaguarROMSize += JaguarCopyImage(vaddr, buffer + offset, filesz);
		WriteLog("FILE: ELF segment %u -> $%08X, %u bytes\n", i, vaddr, filesz);
		loaded++;
	}

	if (loaded == 0)
	{
		WriteLog("FILE: ELF image has no loadable segment\n");
		return false;
	}

	jaguarRunAddress = entry;
	WriteLog("FILE: Setting up ELF 32bits... Run address: %08X\n", entry);
	return true;
}

// ---------------------------------------------------------------------------
// Main load entry point
// ---------------------------------------------------------------------------
bool JaguarLoadFile(char * path)
{
	uint8_t * buffer = nullptr;
	jaguarROMSize = JaguarLoadROM(buffer, path);

	if (jaguarROMSize == 0 || !buffer)
	{
		// It's up to the GUI to report errors, not us. :-)
		WriteLog("FILE: Could not load ROM from file \"%s\"... Aborting load!\n", path);
		return false;
	}

	jaguarMainROMCRC32 = crc32_calcCheckSum(buffer, jaguarROMSize);
	EepromInit();
	jaguarRunAddress = 0x802000;					// For non-BIOS runs, this is true
	const uint32_t fileSize = jaguarROMSize;
	const uint32_t fileType = ParseFileType(buffer, fileSize);
	jaguarCartInserted = false;

	switch (fileType)
	{
	case JST_ROM:
	{
		jaguarCartInserted = true;
		const uint32_t copied = JaguarCopyImage(0x800000, buffer, fileSize);
		jaguarROMSize = copied;
		// The cartridge header reports the real entry point...
		uint32_t reported = copied >= 0x408 ? GET32(jagMemSpace, 0x800000 + 0x404) : 0;
		if (reported == 0 || reported < 0x800000)
		{
			// ...but plenty of dumps leave that field empty.
			reported = 0x802000;
		}
		jaguarRunAddress = reported;
		WriteLog("FILE: Cartridge run address is reported as $%X (size $%X)\n", jaguarRunAddress, fileSize);
		free(buffer);
		return true;
	}

	case JST_ALPINE:
	{
		// No 8 KB header on disk, so the image starts at $802000 and the gap
		// below it is left as $FF. Not a physical cartridge, so the reset
		// vector points straight at the image instead of going through the BIOS.
		WriteLog("FILE: Setting up Alpine ROM... Run address: 00802000, length: %08X\n", fileSize);
		JaguarFillImage(0x800000, 0xFF, 0x2000);
		jaguarROMSize = JaguarCopyImage(0x802000, buffer, fileSize);
		// Point exception #4 at an infinite loop instead of nowhere at all.
		SET32(jaguarMainRAM, 0x10, 0x00001000);
		SET16(jaguarMainRAM, 0x1000, 0x60FE);		// bra.s *
		free(buffer);
		return true;
	}

	case JST_ELF32:
	{
		const bool ok = loadELF32(buffer, fileSize);
		free(buffer);
		return ok;
	}

	case JST_ABS_TYPE1:
	{
		// For ABS type 1, run address == load address
		const uint32_t loadAddress = GET32(buffer, 0x16);
		const uint32_t codeSize = GET32(buffer, 0x02) + GET32(buffer, 0x06);
		WriteLog("FILE: Setting up homebrew (ABS-1)... Run address: %08X, length: %08X\n", loadAddress, codeSize);
		if (0x24 + codeSize > fileSize)
		{
			WriteLog("FILE: ABS-1 code runs past the end of the file\n");
			free(buffer);
			return false;
		}
		jaguarROMSize = JaguarCopyImage(loadAddress, buffer + 0x24, codeSize);
		jaguarRunAddress = loadAddress;
		free(buffer);
		return true;
	}

	case JST_ABS_TYPE2:
	{
		const uint32_t loadAddress = GET32(buffer, 0x28);
		const uint32_t runAddress = GET32(buffer, 0x24);
		const uint32_t codeSize = GET32(buffer, 0x18) + GET32(buffer, 0x1C);
		WriteLog("FILE: Setting up homebrew (ABS-2)... Run address: %08X, length: %08X\n", runAddress, codeSize);
		if (0xA8 + codeSize > fileSize)
		{
			WriteLog("FILE: ABS-2 code runs past the end of the file\n");
			free(buffer);
			return false;
		}
		jaguarROMSize = JaguarCopyImage(loadAddress, buffer + 0xA8, codeSize);
		jaguarRunAddress = runAddress;
		free(buffer);
		return true;
	}

	case JST_JAGSERVER:
	{
		const uint32_t loadAddress = GET32(buffer, 0x22);
		const uint32_t runAddress = GET32(buffer, 0x2A);
		WriteLog("FILE: Setting up homebrew (Jag Server)... Run address: $%X, length: $%X\n",
			runAddress, fileSize - 0x2E);
		if (0x2E > fileSize)
		{
			free(buffer);
			return false;
		}
		jaguarROMSize = JaguarCopyImage(loadAddress, buffer + 0x2E, fileSize - 0x2E);
		jaguarRunAddress = runAddress;
		free(buffer);
		SET32(jaguarMainRAM, 0x10, 0x00001000);
		SET16(jaguarMainRAM, 0x1000, 0x60FE);
		return true;
	}

	case JST_WTFOMGBBQ:
	{
		const uint32_t loadAddress = (buffer[0x1F] << 24) | (buffer[0x1E] << 16) |
			(buffer[0x1D] << 8) | buffer[0x1C];
		WriteLog("FILE: Setting up homebrew (GEMDOS WTFOMGBBQ type)... Run address: $%X, length: $%X\n",
			loadAddress, fileSize - 0x20);
		if (0x20 > fileSize)
		{
			free(buffer);
			return false;
		}
		jaguarROMSize = JaguarCopyImage(loadAddress, buffer + 0x20, fileSize - 0x20);
		jaguarRunAddress = loadAddress;
		free(buffer);
		return true;
	}

	default:
		break;
	}

	WriteLog("FILE: Failed to load unknown format (type %u, size %u).\n", fileType, fileSize);
	free(buffer);
	return false;
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
