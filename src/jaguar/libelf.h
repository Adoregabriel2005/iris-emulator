#pragma once
// Stub libelf.h — ELF loading is not used in Iris headless mode.
// Provides just enough to let file.cpp compile without the real libelf library.

typedef void Elf;
typedef void Elf_Scn;
typedef struct { unsigned char* d_buf; unsigned long d_size; } Elf_Data;
typedef struct { unsigned long sh_type; unsigned long sh_flags; unsigned long sh_addr;
                 unsigned long sh_offset; unsigned long sh_size; unsigned long sh_link;
                 unsigned long sh_info; unsigned long sh_addralign; unsigned long sh_entsize; } GElf_Shdr;
typedef struct { unsigned long st_value; unsigned long st_size; unsigned char st_info;
                 unsigned char st_other; unsigned short st_shndx; char st_name[256]; } GElf_Sym;

#define ELF_C_READ  0
#define ELF_T_BYTE  0
#define SHT_SYMTAB  2
#define SHT_STRTAB  3
#define SHF_ALLOC   2
#define ELF32_ST_TYPE(i) 0
#define STT_FUNC    2

static inline int          elf_version(int v)                          { (void)v; return 0; }
static inline Elf*         elf_begin(int fd, int cmd, Elf* ref)        { (void)fd;(void)cmd;(void)ref; return nullptr; }
static inline int          elf_end(Elf* e)                             { (void)e; return 0; }
static inline Elf_Scn*     elf_nextscn(Elf* e, Elf_Scn* s)            { (void)e;(void)s; return nullptr; }
static inline int          gelf_getshdr(Elf_Scn* s, GElf_Shdr* h)     { (void)s;(void)h; return 0; }
static inline Elf_Data*    elf_getdata(Elf_Scn* s, Elf_Data* d)       { (void)s;(void)d; return nullptr; }
static inline int          gelf_getsym(Elf_Data* d, int i, GElf_Sym* s){ (void)d;(void)i;(void)s; return 0; }
static inline const char*  elf_strptr(Elf* e, int i, unsigned long o)  { (void)e;(void)i;(void)o; return ""; }
static inline int          gelf_getclass(Elf* e)                       { (void)e; return 0; }
