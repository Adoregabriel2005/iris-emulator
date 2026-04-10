#pragma once
#include "MapperBanked.h"

// F6: 16KB ROM, 4x4KB banks. Hotspots: 0x1FF6=bank0..0x1FF9=bank3.
class MapperF6 : public MapperBanked {
public:
    explicit MapperF6(const QByteArray *romData)
        : MapperBanked(romData, 0x1000, std::vector<uint16_t>{0x1FF6, 0x1FF7, 0x1FF8, 0x1FF9}) {}
};
