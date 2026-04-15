#pragma once
#include "MapperBanked.h"

// F4: 32KB ROM, 8x4KB banks. Hotspots: 0x1FF4=bank0..0x1FFB=bank7.
class MapperF4 : public MapperBanked {
public:
    explicit MapperF4(const QByteArray *romData)
        : MapperBanked(romData, 0x1000, std::vector<uint16_t>{
            0x1FF4, 0x1FF5, 0x1FF6, 0x1FF7,
            0x1FF8, 0x1FF9, 0x1FFA, 0x1FFB}) {}
};
