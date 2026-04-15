#pragma once

#include <cstdint>
#include <QByteArray>
#include <QDataStream>

class Mapper {
public:
    explicit Mapper(const QByteArray *romData) : rom(romData) {}
    virtual ~Mapper() {}

    // Read from CPU address space (0x0000 - 0x1FFF for 6507)
    virtual uint8_t read(uint16_t addr) const = 0;
    // CPU write to address space — mappers often observe writes to certain addresses
    virtual void write(uint16_t addr, uint8_t value) = 0;

    virtual void reset() {}

    virtual void serialize(QDataStream &ds) const {}
    virtual void deserialize(QDataStream &ds) {}

protected:
    const QByteArray *rom;
};
