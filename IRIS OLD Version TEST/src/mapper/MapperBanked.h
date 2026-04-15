#pragma once
#include "Mapper.h"
#include <vector>
#include <cstring>

// Atari 2600 banked mapper.
// controlAddrs[i] is the hotspot address that selects bank i.
// Both reads AND writes to hotspot addresses trigger bank switches.
// Optional Superchip: 128 bytes of extra RAM at $1000-$10FF
//   Write: $1000-$107F, Read: $1080-$10FF
class MapperBanked : public Mapper {
public:
    MapperBanked(const QByteArray *romData, int bankSizeBytes, const std::vector<uint16_t> &controlAddrs, bool superchip = false);
    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;
    void reset() override;
    void serialize(QDataStream &ds) const override;
    void deserialize(QDataStream &ds) override;

private:
    void checkHotspot(uint16_t addr) const;
    int bankSize;
    int bankCount;
    mutable int currentBank; // mutable: reads can trigger bank switches
    std::vector<uint16_t> controlAddrs;
    bool hasSuperchip;
    mutable uint8_t scRAM[128]; // Superchip extra RAM
};
