#pragma once

#include <cstdint>
#include <vector>
#include <QString>
#include <QDataStream>

// Atari Lynx cartridge — .lnx file parser and shift-register addressed ROM.
// Address model: addr = (shifter << shiftCount) + (counter & countMask).
// Suzy reads/writes RCART0/RCART1 → peek0/poke0, peek1/poke1.
// Mikey IODAT/SYSCTL1 control address strobe and data lines.
class LynxCart {
public:
    LynxCart();

    bool loadFromFile(const QString &path);

    // Suzy RCART0/RCART1: read/write byte, auto-increment counter
    uint8_t peek0();
    uint8_t peek1();
    void poke0(uint8_t data);
    void poke1(uint8_t data);

    // Cart address shift register — controlled by Mikey IODAT/SYSCTL1
    void cartAddressStrobe(bool strobe);
    void cartAddressData(bool data);
    void setWriteEnableBank1(bool en) { m_write_enable_bank1 = en; }

    // Debug accessors
    bool strobeState() const { return m_strobe; }
    uint32_t shifterVal() const { return m_shifter; }
    uint32_t counterVal() const { return m_counter; }

    // Header info
    QString cartName() const { return m_cart_name; }
    QString manufacturer() const { return m_manufacturer; }
    uint8_t rotation() const { return m_rotation; }
    int bank0Size() const { return m_bank0_size; }
    int bank1Size() const { return m_bank1_size; }

    void serialize(QDataStream &ds) const;
    void deserialize(QDataStream &ds);

private:
    std::vector<uint8_t> m_bank0;
    std::vector<uint8_t> m_bank1;
    int m_bank0_size = 0;
    int m_bank1_size = 0;

    uint32_t m_shifter = 0;
    uint32_t m_counter = 0;
    bool m_addr_data = false;
    bool m_strobe = false;
    bool m_last_strobe = false;

    uint32_t m_mask_bank0 = 0;
    int m_shift_count0 = 0;
    uint32_t m_count_mask0 = 0;

    uint32_t m_mask_bank1 = 0;
    int m_shift_count1 = 0;
    uint32_t m_count_mask1 = 0;

    bool m_write_enable_bank1 = false;

    QString m_cart_name;
    QString m_manufacturer;
    uint8_t m_rotation = 0;
};
