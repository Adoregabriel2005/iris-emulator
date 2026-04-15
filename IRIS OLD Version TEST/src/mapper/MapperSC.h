#pragma once
#include "MapperBanked.h"

// F8SC: 8KB ROM + 128B Superchip RAM, 2x4KB banks
class MapperF8SC : public MapperBanked {
public:
    explicit MapperF8SC(const QByteArray *romData)
        : MapperBanked(romData, 0x1000, std::vector<uint16_t>{0x1FF8, 0x1FF9}, true) {}
};

// F6SC: 16KB ROM + 128B Superchip RAM, 4x4KB banks
class MapperF6SC : public MapperBanked {
public:
    explicit MapperF6SC(const QByteArray *romData)
        : MapperBanked(romData, 0x1000, std::vector<uint16_t>{0x1FF6, 0x1FF7, 0x1FF8, 0x1FF9}, true) {}
};

// F4SC: 32KB ROM + 128B Superchip RAM, 8x4KB banks
class MapperF4SC : public MapperBanked {
public:
    explicit MapperF4SC(const QByteArray *romData)
        : MapperBanked(romData, 0x1000, std::vector<uint16_t>{
            0x1FF4, 0x1FF5, 0x1FF6, 0x1FF7,
            0x1FF8, 0x1FF9, 0x1FFA, 0x1FFB}, true) {}
};
