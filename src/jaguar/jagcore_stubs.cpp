// Stubs e variáveis essenciais do core Jaguar (adaptado do Virtual Jaguar)
#include <cstdint>

// Exportados para patch/hack/debug
unsigned int jaguarMainROMCRC32 = 0;
unsigned int jaguarRunAddress = 0;

// Simulação mínima do core 68k (BigPEmu/VirtualJaguar)
extern "C" int m68k_get_reg(void* context, int regnum) {
    // Retorne 0 ou valor dummy; adapte conforme necessário
    return 0;
}
