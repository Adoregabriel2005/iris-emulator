#pragma once
#include "MapperBanked.h"

class MapperE7 : public MapperBanked {
public:
    explicit MapperE7(const QByteArray *romData)
        : MapperBanked(romData, 0x1000, std::vector<uint16_t>{0x1FE7}) {}
};
