
#include "EmulatorCore.h"
#include <QFile>
#include <QPainter>
#include <QFileInfo>
#include <QDebug>
#include <QCryptographicHash>
#include <QBuffer>
#include <QCoreApplication>

EmulatorCore::EmulatorCore(QObject *parent)
    : IEmulatorCore(parent)
{
    tia = new TIA();
    riot = new RIOT();
    memset(ram128, 0, sizeof(ram128));
    frameBuffer = QImage(160, 312, QImage::Format_RGB32);
    frameBuffer.fill(Qt::black);
}

EmulatorCore::~EmulatorCore()
{
    closeAudio();
    delete cpu;
    delete tia;
    delete riot;
}

bool EmulatorCore::loadROM(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    romData = f.readAll();
    romPath = path;

    // Create CPU with bus callbacks
    auto rcb = [this](uint16_t addr)->uint8_t { return this->busRead(addr); };
    auto wcb = [this](uint16_t addr, uint8_t v) { this->busWrite(addr, v); };
    if (cpu) delete cpu;
    cpu = new CPU6507(rcb, wcb);

    // Cycle callback: advance TIA by 3 color clocks per CPU bus access
    // This keeps TIA in sync with the CPU during instruction execution
    cpu->cycleCallback = [this]() {
        if (tia) {
            tia->tickSingleClock();
            tia->tickSingleClock();
            tia->tickSingleClock();
            m_tiaClocksAdvanced += 3;
        }
    };

    // compute SHA1 and create mapper
    QString sha1;
    {
        QCryptographicHash h(QCryptographicHash::Sha1);
        h.addData(romData);
        sha1 = QString::fromUtf8(h.result().toHex());
    }
    mapper = MapperFactory::createMapperForROM(&romData, sha1, QCoreApplication::applicationDirPath());

    if (mapper) mapper->reset();

    return true;
}

void EmulatorCore::start()
{
    if (!cpu) return;
    cpu->reset();
    m_running = true;
}

void EmulatorCore::stop()
{
    m_running = false;
}

void EmulatorCore::setTVStandard(TVStandard std)
{
    if (tia) tia->setTVStandard(std);
    // NTSC: ~19886 cpu cycles/frame (262 scanlines * 76 cycles)
    // PAL/SECAM: ~23712 cpu cycles/frame (312 scanlines * 76 cycles)
    cyclesPerFrame = (std == TVStandard::NTSC) ? 19912 : 23712;
}

TVStandard EmulatorCore::getTVStandard() const
{
    return tia ? tia->getTVStandard() : TVStandard::NTSC;
}

uint8_t EmulatorCore::busRead(uint16_t addr) const
{
    addr &= 0x1FFF; // 13-bit address bus

    // A12=1: ROM / mapper
    if (addr & 0x1000) {
        if (mapper) return mapper->read(addr);
        if (romData.isEmpty()) return 0;
        uint32_t idx = (addr - 0x1000) % (uint32_t)romData.size();
        return static_cast<uint8_t>(romData.at(idx));
    }

    // A12=0, A7=0: TIA (0x0000-0x007F and mirrors like 0x0100-0x017F etc.)
    if (!(addr & 0x0080)) {
        return tia->read(addr & 0x0F);
    }

    // A12=0, A7=1, A9=1: RIOT registers (0x0280-0x02FF and mirrors)
    if (addr & 0x0200) {
        return riot->read(addr);
    }

    // A12=0, A7=1, A9=0: RAM (0x0080-0x00FF and mirrors like 0x0180-0x01FF)
    return ram128[addr & 0x7F];
}

void EmulatorCore::busWrite(uint16_t addr, uint8_t val)
{
    addr &= 0x1FFF; // 13-bit address bus

    // A12=1: ROM / mapper (mappers observe writes for bank switching)
    if (addr & 0x1000) {
        if (mapper) mapper->write(addr, val);
        return;
    }

    // A12=0, A7=0: TIA
    if (!(addr & 0x0080)) {
        tia->lastDataBus = val;
        tia->write(addr & 0x3F, val);
        return;
    }

    // A12=0, A7=1, A9=1: RIOT registers
    if (addr & 0x0200) {
        riot->write(addr, val);
        return;
    }

    // A12=0, A7=1, A9=0: RAM
    ram128[addr & 0x7F] = val;
}

void EmulatorCore::step()
{
    if (!m_running || !cpu) return;
    int cycles = 0;
    while (cycles < cyclesPerFrame) {
        if (tia && tia->wsyncHalt()) {
            int wsyncClocks = 0;
            while (tia->wsyncHalt() && wsyncClocks < 228) {
                tia->tickSingleClock();
                wsyncClocks++;
                if (wsyncClocks % 3 == 0 && riot)
                    riot->tick(1);
            }
            if (tia->wsyncHalt()) tia->clearWsync();
            cycles += (wsyncClocks + 2) / 3;
            continue;
        }
        m_tiaClocksAdvanced = 0;
        int c = cpu->step();
        cycles += c;
        int remaining = c * 3 - m_tiaClocksAdvanced;
        for (int i = 0; i < remaining; ++i)
            tia->tickSingleClock();
        if (riot) riot->tick(c);
    }
    if (tia) frameBuffer = tia->getFrame();
}

QImage EmulatorCore::getFrame() const
{
    return frameBuffer;
}

bool EmulatorCore::saveState(const QString &path)
{
    QByteArray data = dumpState();
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return false;
    qint64 written = f.write(data);
    return written == data.size();
}

bool EmulatorCore::loadState(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return false;
    QByteArray data = f.readAll();
    return loadStateFromData(data);
}

QByteArray EmulatorCore::dumpState() const
{
    QByteArray out;
    QDataStream ds(&out, QIODevice::WriteOnly);
    ds << romPath;
    ds << romData;
    // CPU
    if (cpu) cpu->serialize(ds);
    else {
        // write empty defaults
        CPU6507 tmp([](uint16_t){return uint8_t(0);}, [](uint16_t, uint8_t){});
        tmp.serialize(ds);
    }
    // RAM
    for (int i = 0; i < 128; ++i) ds << ram128[i];
    // TIA
    if (tia) tia->serialize(ds);
    else {
        TIA tmp;
        tmp.serialize(ds);
    }
    // RIOT
    if (riot) riot->serialize(ds);
    else {
        RIOT tmp;
        tmp.serialize(ds);
    }
    // Mapper state
    if (mapper) mapper->serialize(ds);
    return out;
}

bool EmulatorCore::loadStateFromData(const QByteArray &data)
{
    QBuffer buffer;
    buffer.setData(data);
    if (!buffer.open(QIODevice::ReadOnly)) return false;
    QDataStream ds(&buffer);
    ds >> romPath;
    ds >> romData;

    // create mapper based on romData + sha1
    QString sha1;
    {
        QCryptographicHash h(QCryptographicHash::Sha1);
        h.addData(romData);
        sha1 = QString::fromUtf8(h.result().toHex());
    }
    // recreate mapper for this ROM
    mapper = MapperFactory::createMapperForROM(&romData, sha1, QCoreApplication::applicationDirPath());
    if (mapper) mapper->reset();

    // create CPU with bus callbacks (mapper must exist before CPU construction)
    if (cpu) { delete cpu; cpu = nullptr; }
    auto rcb = [this](uint16_t addr)->uint8_t { return this->busRead(addr); };
    auto wcb = [this](uint16_t addr, uint8_t v) { this->busWrite(addr, v); };
    cpu = new CPU6507(rcb, wcb);

    // Cycle callback: advance TIA by 3 color clocks per CPU bus access
    cpu->cycleCallback = [this]() {
        if (tia) {
            tia->tickSingleClock();
            tia->tickSingleClock();
            tia->tickSingleClock();
            m_tiaClocksAdvanced += 3;
        }
    };

    cpu->deserialize(ds);

    for (int i = 0; i < 128; ++i) ds >> ram128[i];

    if (!tia) tia = new TIA();
    tia->deserialize(ds);
    if (!riot) riot = new RIOT();
    riot->deserialize(ds);

    // mapper deserialize best-effort
    if (mapper) mapper->deserialize(ds);

    return true;
}

// ════════════════════════════════════════════════════════════════════════
// Audio output via SDL
// ════════════════════════════════════════════════════════════════════════

void EmulatorCore::audioCallback(void* userdata, Uint8* stream, int len)
{
    auto* core = static_cast<EmulatorCore*>(userdata);
    int samples = len / sizeof(int16_t);
    auto* out = reinterpret_cast<int16_t*>(stream);

    if (!core->m_audio_enabled || !core->tia) {
        memset(stream, 0, len);
        return;
    }

    core->tia->readAudioBuffer(out, samples);

    // Apply volume
    if (core->m_audio_volume < 100) {
        int vol = core->m_audio_volume;
        for (int i = 0; i < samples; i++) {
            out[i] = static_cast<int16_t>((int)out[i] * vol / 100);
        }
    }
}

void EmulatorCore::initAudio(const QString& deviceName)
{
    closeAudio();

    SDL_AudioSpec desired{};
    SDL_AudioSpec obtained{};

    desired.freq = m_audio_sample_rate;
    desired.format = AUDIO_S16SYS;
    desired.channels = 1; // mono
    desired.samples = 1024;
    desired.callback = audioCallback;
    desired.userdata = this;

    const char* devName = nullptr;
    QByteArray devNameUtf8;
    if (!deviceName.isEmpty() && deviceName != "System Default" && deviceName != "default") {
        devNameUtf8 = deviceName.toUtf8();
        devName = devNameUtf8.constData();
    }

    m_audio_device = SDL_OpenAudioDevice(devName, 0, &desired, &obtained, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (m_audio_device == 0) {
        qWarning() << "SDL_OpenAudioDevice failed:" << SDL_GetError();
        return;
    }

    m_audio_sample_rate = obtained.freq;
    if (tia) tia->setAudioSampleRate(m_audio_sample_rate);

    qDebug() << "Audio device opened:" << (devName ? devName : "default")
             << "rate:" << obtained.freq << "channels:" << obtained.channels
             << "samples:" << obtained.samples;

    // Start playback
    SDL_PauseAudioDevice(m_audio_device, 0);
}

void EmulatorCore::closeAudio()
{
    if (m_audio_device != 0) {
        SDL_CloseAudioDevice(m_audio_device);
        m_audio_device = 0;
    }
}

void EmulatorCore::setAudioVolume(int percent)
{
    m_audio_volume = std::clamp(percent, 0, 100);
}

void EmulatorCore::setAudioEnabled(bool enabled)
{
    m_audio_enabled = enabled;
    if (m_audio_device != 0) {
        SDL_PauseAudioDevice(m_audio_device, enabled ? 0 : 1);
    }
}

void EmulatorCore::setJoystickState(const JoystickState &s)
{
    if (riot) {
        riot->setJoystickState(s);
        riot->setSelect(s.select);
        riot->setReset(s.reset);
        // Edge-triggered toggles (only on press, not hold)
        if (s.p0DiffToggle && !m_prev_js.p0DiffToggle)
            riot->setP0Diff(!riot->getP0Diff());
        if (s.colorBWToggle && !m_prev_js.colorBWToggle)
            riot->setColorBW(!riot->getColorMode());
    }
    if (tia) {
        tia->setFireButton(0, s.fire);
        tia->setFireButton(1, s.p1fire);
    }
    m_prev_js = s;
}

