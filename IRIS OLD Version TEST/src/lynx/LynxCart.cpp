#include "LynxCart.h"
#include <QFile>
#include <QDebug>
#include <cstring>

LynxCart::LynxCart() = default;

// Compute shift count and masks for a given bank page count
static void computeBankParams(uint16_t pages, uint32_t &mask, int &shiftCount, uint32_t &countMask)
{
    switch (pages) {
    case 0x000: mask = 0;        shiftCount = 0;  countMask = 0;      break;
    case 0x100: mask = 0x00FFFF; shiftCount = 8;  countMask = 0x0FF;  break; // 64K
    case 0x200: mask = 0x01FFFF; shiftCount = 9;  countMask = 0x1FF;  break; // 128K
    case 0x400: mask = 0x03FFFF; shiftCount = 10; countMask = 0x3FF;  break; // 256K
    case 0x800: mask = 0x07FFFF; shiftCount = 11; countMask = 0x7FF;  break; // 512K
    default:    mask = 0x00FFFF; shiftCount = 8;  countMask = 0x0FF;  break;
    }
}

bool LynxCart::loadFromFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "LynxCart: cannot open" << path;
        return false;
    }

    QByteArray data = f.readAll();
    if (data.size() < 10) {
        qWarning() << "LynxCart: file too small";
        return false;
    }

    const uint8_t *hdr = reinterpret_cast<const uint8_t*>(data.constData());
    int headerSize = 0;
    uint16_t bank0Pages = 0, bank1Pages = 0;

    if (hdr[0] == 'L' && hdr[1] == 'Y' && hdr[2] == 'N' && hdr[3] == 'X') {
        headerSize = 64;
        bank0Pages = hdr[4] | (hdr[5] << 8);
        bank1Pages = hdr[6] | (hdr[7] << 8);

        char name[33] = {};
        std::memcpy(name, hdr + 0x0A, 32);
        m_cart_name = QString::fromLatin1(name).trimmed();

        char mfg[17] = {};
        std::memcpy(mfg, hdr + 0x2A, 16);
        m_manufacturer = QString::fromLatin1(mfg).trimmed();

        m_rotation = hdr[0x3A];
    } else {
        // Raw ROM — guess bank size from file size
        headerSize = 0;
        bank0Pages = data.size() >> 8;
        bank1Pages = 0;
        m_cart_name = QStringLiteral("Unknown");
        m_manufacturer.clear();
        m_rotation = 0;
    }

    m_bank0_size = bank0Pages * 256;
    m_bank1_size = bank1Pages * 256;

    computeBankParams(bank0Pages, m_mask_bank0, m_shift_count0, m_count_mask0);
    computeBankParams(bank1Pages, m_mask_bank1, m_shift_count1, m_count_mask1);

    // Allocate and fill bank 0
    if (m_mask_bank0) {
        m_bank0.resize(m_mask_bank0 + 1, 0xFF);
        int avail = data.size() - headerSize;
        int toCopy = std::min<int>(avail, (int)m_bank0.size());
        if (toCopy > 0)
            std::memcpy(m_bank0.data(), data.constData() + headerSize, toCopy);
    }

    // Allocate and fill bank 1
    if (m_mask_bank1) {
        m_bank1.resize(m_mask_bank1 + 1, 0xFF);
        int bank0DataLen = std::min<int>(data.size() - headerSize, (int)(m_mask_bank0 + 1));
        int offset = headerSize + bank0DataLen;
        int avail = data.size() - offset;
        int toCopy = std::min<int>(avail, (int)m_bank1.size());
        if (toCopy > 0)
            std::memcpy(m_bank1.data(), data.constData() + offset, toCopy);
    } else {
        // No bank 1 in file — create 64K shadow SRAM
        m_mask_bank1 = 0x00FFFF;
        m_shift_count1 = 8;
        m_count_mask1 = 0x0FF;
        m_bank1.resize(m_mask_bank1 + 1, 0xFF);
        m_write_enable_bank1 = true;
    }

    m_shifter = 0;
    m_counter = 0;
    m_strobe = false;
    m_last_strobe = false;
    m_addr_data = false;

    qDebug() << "LynxCart: loaded" << m_cart_name
             << "bank0:" << m_bank0_size << "bank1:" << m_bank1_size
             << "shift0:" << m_shift_count0 << "mask0:" << Qt::hex << m_count_mask0
             << "rotation:" << m_rotation;
    return !m_bank0.empty();
}

uint8_t LynxCart::peek0()
{
    if (m_bank0.empty()) return 0xFF;
    uint32_t addr = (m_shifter << m_shift_count0) + (m_counter & m_count_mask0);
    uint8_t val = m_bank0[addr & m_mask_bank0];
    if (!m_strobe) {
        m_counter++;
        m_counter &= 0x07FF;
    }
    return val;
}

uint8_t LynxCart::peek1()
{
    if (m_bank1.empty()) return 0xFF;
    uint32_t addr = (m_shifter << m_shift_count1) + (m_counter & m_count_mask1);
    uint8_t val = m_bank1[addr & m_mask_bank1];
    if (!m_strobe) {
        m_counter++;
        m_counter &= 0x07FF;
    }
    return val;
}

void LynxCart::poke0(uint8_t /*data*/)
{
    // Bank 0 is ROM — writes ignored
    if (!m_strobe) {
        m_counter++;
        m_counter &= 0x07FF;
    }
}

void LynxCart::poke1(uint8_t data)
{
    if (m_write_enable_bank1 && !m_bank1.empty()) {
        uint32_t addr = (m_shifter << m_shift_count1) + (m_counter & m_count_mask1);
        m_bank1[addr & m_mask_bank1] = data;
    }
    if (!m_strobe) {
        m_counter++;
        m_counter &= 0x07FF;
    }
}

void LynxCart::cartAddressStrobe(bool strobe)
{
    m_strobe = strobe;
    if (m_strobe) m_counter = 0;
    // Shift data bit on rising edge of strobe (matching Handy)
    if (m_strobe && !m_last_strobe) {
        m_shifter = (m_shifter << 1) | (m_addr_data ? 1 : 0);
        m_shifter &= 0xFF;
    }
    m_last_strobe = m_strobe;
}

void LynxCart::cartAddressData(bool data)
{
    // Just store the data line state — shifting happens in cartAddressStrobe
    m_addr_data = data;
}

void LynxCart::serialize(QDataStream &ds) const
{
    ds << m_shifter << m_counter << m_addr_data << m_strobe << m_last_strobe;
    // Bank 1 SRAM
    if (m_write_enable_bank1)
        ds.writeRawData(reinterpret_cast<const char*>(m_bank1.data()), (int)m_bank1.size());
}

void LynxCart::deserialize(QDataStream &ds)
{
    ds >> m_shifter >> m_counter >> m_addr_data >> m_strobe >> m_last_strobe;
    if (m_write_enable_bank1)
        ds.readRawData(reinterpret_cast<char*>(m_bank1.data()), (int)m_bank1.size());
}
