#pragma once
#include "Mapper.h"

class MapperNone : public Mapper {
public:
    explicit MapperNone(const QByteArray *romData);
    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;
};
