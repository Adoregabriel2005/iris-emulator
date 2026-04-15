#pragma once

#include <memory>
#include <QString>

class Mapper;
class MapperFactory {
public:
    // Create a mapper instance for the given ROM data and SHA1 (if available).
    // appDir is used to locate data/mappers.json for known mappings.
    static std::unique_ptr<class Mapper> createMapperForROM(const QByteArray *rom, const QString &sha1, const QString &appDir);
};
