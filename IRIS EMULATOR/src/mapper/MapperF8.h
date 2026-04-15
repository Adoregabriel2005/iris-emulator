#pragma once
#include "MapperBanked.h"

// F8: 8KB ROM, 2x4KB banks. Hotspots: 0x1FF8=bank0, 0x1FF9=bank1.
class MapperF8 : public MapperBanked {
public:
    explicit MapperF8(const QByteArray *romData)
        : MapperBanked(romData, 0x1000, std::vector<uint16_t>{0x1FF8, 0x1FF9}) {}
};
