#include "MapperNone.h"

MapperNone::MapperNone(const QByteArray *romData)
    : Mapper(romData)
{
}

uint8_t MapperNone::read(uint16_t addr) const
{
    addr &= 0x1FFF;
    if (addr >= 0x1000) {
        if (!rom || rom->isEmpty()) return 0;
        uint32_t idx = (addr - 0x1000) % static_cast<uint32_t>(rom->size());
        return static_cast<uint8_t>(rom->at(idx));
    }
    return 0;
}

void MapperNone::write(uint16_t addr, uint8_t value)
{
    Q_UNUSED(addr);
    Q_UNUSED(value);
    // no-op for ROM writes
}
