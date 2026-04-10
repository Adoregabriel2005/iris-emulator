#include "MapperBanked.h"
#include <QDebug>

MapperBanked::MapperBanked(const QByteArray *romData, int bankSizeBytes, const std::vector<uint16_t> &controlAddrs, bool superchip)
    : Mapper(romData), bankSize(bankSizeBytes), currentBank(0), controlAddrs(controlAddrs), hasSuperchip(superchip)
{
    memset(scRAM, 0, sizeof(scRAM));
    if (bankSize <= 0) bankSize = 0x1000;
    if (!rom || rom->isEmpty()) {
        bankCount = 1;
    } else {
        bankCount = static_cast<int>((rom->size() + bankSize - 1) / bankSize);
        if (bankCount < 1) bankCount = 1;
    }
}

// Check if addr matches a hotspot — if so, switch to corresponding bank.
// controlAddrs[i] selects bank i.
void MapperBanked::checkHotspot(uint16_t addr) const
{
    for (size_t i = 0; i < controlAddrs.size(); ++i) {
        if ((controlAddrs[i] & 0x1FFF) == addr) {
            int newBank = static_cast<int>(i) % bankCount;
            if (newBank != currentBank) {
                currentBank = newBank;
            }
            return;
        }
    }
}

uint8_t MapperBanked::read(uint16_t addr) const
{
    addr &= 0x1FFF;
    // Bank switching triggered on reads too (Atari 2600 hotspot)
    checkHotspot(addr);

    if (addr >= 0x1000) {
        // Superchip read area: $1080-$10FF
        if (hasSuperchip && addr >= 0x1080 && addr < 0x1100) {
            return scRAM[addr - 0x1080];
        }
        // Superchip write area: reads from $1000-$107F return 0 (write-only)
        if (hasSuperchip && addr >= 0x1000 && addr < 0x1080) {
            return 0;
        }
        if (!rom || rom->isEmpty()) return 0;
        int offset = addr - 0x1000;
        int idx = currentBank * bankSize + offset;
        if (idx < 0) return 0;
        idx %= rom->size();
        return static_cast<uint8_t>(rom->at(idx));
    }
    return 0;
}

void MapperBanked::write(uint16_t addr, uint8_t value)
{
    addr &= 0x1FFF;
    // Bank switching triggered on writes (Atari 2600 hotspot)
    checkHotspot(addr);
    // Superchip write area: $1000-$107F
    if (hasSuperchip && addr >= 0x1000 && addr < 0x1080) {
        scRAM[addr - 0x1000] = value;
    }
}

void MapperBanked::reset()
{
    // Most F8 ROMs start in the LAST bank (bank 1 for 8KB)
    currentBank = bankCount > 1 ? bankCount - 1 : 0;
}

void MapperBanked::serialize(QDataStream &ds) const
{
    ds << static_cast<qint32>(bankSize);
    ds << static_cast<qint32>(bankCount);
    ds << static_cast<qint32>(currentBank);
    ds << hasSuperchip;
    if (hasSuperchip) {
        for (int i = 0; i < 128; i++) ds << scRAM[i];
    }
}

void MapperBanked::deserialize(QDataStream &ds)
{
    qint32 bs = 0, bc = 0, cb = 0;
    ds >> bs;
    ds >> bc;
    ds >> cb;
    bankSize = bs;
    bankCount = bc;
    currentBank = cb;
    bool sc = false;
    ds >> sc;
    hasSuperchip = sc;
    if (hasSuperchip) {
        for (int i = 0; i < 128; i++) ds >> scRAM[i];
    }
}
