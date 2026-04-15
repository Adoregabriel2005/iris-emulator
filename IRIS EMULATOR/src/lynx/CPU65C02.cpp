#include "CPU65C02.h"

CPU65C02::CPU65C02(ReadFunc r, WriteFunc w) : read(r), write(w) {}
CPU65C02::~CPU65C02() = default;

void CPU65C02::reset()
{
    A = 0; X = 0; Y = 0;
    SP = 0xFD;
    P = 0x24; // IRQ disabled
    PC = rd16(0xFFFC);
    halted = false;
    m_nmi_pending = false;
    m_irq_pending = false;
    m_nmi_edge = false;
}

void CPU65C02::triggerNMI()
{
    m_nmi_pending = true;
}

void CPU65C02::triggerIRQ()
{
    m_irq_pending = true;
}

void CPU65C02::serialize(QDataStream &ds) const
{
    ds << A << X << Y << P << SP << PC << halted;
}

void CPU65C02::deserialize(QDataStream &ds)
{
    ds >> A >> X >> Y >> P >> SP >> PC >> halted;
}

// ── Addressing modes ──

uint8_t CPU65C02::fetch() { return rd(PC++); }

uint16_t CPU65C02::addr_zp()    { return fetch(); }
uint16_t CPU65C02::addr_zp_x()  { return (fetch() + X) & 0xFF; }
uint16_t CPU65C02::addr_zp_y()  { return (fetch() + Y) & 0xFF; }
uint16_t CPU65C02::addr_abs()   { uint8_t lo = fetch(); uint8_t hi = fetch(); return (hi << 8) | lo; }

uint16_t CPU65C02::addr_abs_x(bool &pageCross) {
    uint16_t base = addr_abs();
    uint16_t addr = base + X;
    pageCross = ((base ^ addr) & 0xFF00) != 0;
    return addr;
}

uint16_t CPU65C02::addr_abs_y(bool &pageCross) {
    uint16_t base = addr_abs();
    uint16_t addr = base + Y;
    pageCross = ((base ^ addr) & 0xFF00) != 0;
    return addr;
}

uint16_t CPU65C02::addr_ind_x() {
    uint8_t zp = (fetch() + X) & 0xFF;
    uint8_t lo = rd(zp);
    uint8_t hi = rd((zp + 1) & 0xFF);
    return (hi << 8) | lo;
}

uint16_t CPU65C02::addr_ind_y(bool &pageCross) {
    uint8_t zp = fetch();
    uint8_t lo = rd(zp);
    uint8_t hi = rd((zp + 1) & 0xFF);
    uint16_t base = (hi << 8) | lo;
    uint16_t addr = base + Y;
    pageCross = ((base ^ addr) & 0xFF00) != 0;
    return addr;
}

// 65C02-only: (zp) — zero page indirect without X/Y
uint16_t CPU65C02::addr_zp_ind() {
    uint8_t zp = fetch();
    uint8_t lo = rd(zp);
    uint8_t hi = rd((zp + 1) & 0xFF);
    return (hi << 8) | lo;
}

// 65C02-only: (abs,X) — for JMP
uint16_t CPU65C02::addr_abs_ind_x() {
    uint16_t base = addr_abs();
    uint16_t addr = base + X;
    return rd16(addr);
}

// ── Stack ──

void CPU65C02::push8(uint8_t v)      { wr(0x0100 | SP, v); SP--; }
void CPU65C02::push16(uint16_t v)    { push8(v >> 8); push8(v & 0xFF); }
uint8_t CPU65C02::pull8()            { SP++; return rd(0x0100 | SP); }
uint16_t CPU65C02::pull16()          { uint8_t lo = pull8(); uint8_t hi = pull8(); return (hi << 8) | lo; }

// ── ALU ──

void CPU65C02::op_adc(uint8_t val)
{
    if (getD()) {
        // BCD mode (65C02 fixes: correct N/Z/V flags)
        int lo = (A & 0x0F) + (val & 0x0F) + (getC() ? 1 : 0);
        int hi = (A & 0xF0) + (val & 0xF0);
        if (lo > 9) { lo -= 10; hi += 0x10; }
        // V flag: set before decimal adjustment
        int sum = (int)(int8_t)A + (int)(int8_t)val + (getC() ? 1 : 0);
        setV(sum < -128 || sum > 127);
        int result = hi + (lo & 0x0F);
        if (result > 0x9F) result -= 0xA0;
        if ((hi + (lo & 0x0F)) > 0x9F) { result += 0x60; }
        // Recalculate properly
        int total = (A & 0x0F) + (val & 0x0F) + (getC() ? 1 : 0);
        int carry_lo = 0;
        if (total > 9) { total -= 10; carry_lo = 1; }
        int total_hi = (A >> 4) + (val >> 4) + carry_lo;
        bool carry = total_hi > 9;
        if (total_hi > 9) total_hi -= 10;
        uint8_t res = ((total_hi & 0xF) << 4) | (total & 0xF);
        setC(carry);
        A = res;
        setZN(A);
    } else {
        uint16_t sum = A + val + (getC() ? 1 : 0);
        setC(sum > 0xFF);
        setV(((A ^ sum) & (val ^ sum) & 0x80) != 0);
        A = sum & 0xFF;
        setZN(A);
    }
}

void CPU65C02::op_sbc(uint8_t val)
{
    if (getD()) {
        // BCD subtraction (65C02 fixes N/Z flags)
        int lo = (A & 0x0F) - (val & 0x0F) - (getC() ? 0 : 1);
        int hi = (A >> 4) - (val >> 4);
        if (lo < 0) { lo += 10; hi--; }
        if (hi < 0) { hi += 10; setC(false); } else { setC(true); }
        // V flag from binary
        uint16_t bsum = A - val - (getC() ? 0 : 1);
        setV(((A ^ bsum) & (~val ^ bsum) & 0x80) != 0);
        // Recalculate with correct borrow for V
        int btotal = (int)A - (int)val - (getC() ? 0 : 1);
        // Redo properly for precise 65C02 behavior
        int borrow = getC() ? 0 : 1;
        int lo2 = (A & 0x0F) - (val & 0x0F) - borrow;
        int hi2 = (A >> 4) - (val >> 4);
        if (lo2 < 0) { lo2 += 10; hi2--; }
        bool c = true;
        if (hi2 < 0) { hi2 += 10; c = false; }
        uint8_t res = ((hi2 & 0xF) << 4) | (lo2 & 0xF);
        // Binary result for Z/N/V
        uint16_t binres = (uint16_t)A - val - borrow;
        setV(((A ^ binres) & (A ^ val) & 0x80) != 0);
        setC(c);
        A = res;
        setZN((uint8_t)(binres & 0xFF)); // 65C02: Z/N from binary
    } else {
        uint16_t diff = A - val - (getC() ? 0 : 1);
        setC(diff < 0x100);
        setV(((A ^ diff) & (A ^ val) & 0x80) != 0);
        A = diff & 0xFF;
        setZN(A);
    }
}

void CPU65C02::op_cmp(uint8_t reg, uint8_t val)
{
    uint16_t diff = reg - val;
    setC(reg >= val);
    setZN(diff & 0xFF);
}

int CPU65C02::doBranch(bool cond)
{
    int8_t offset = static_cast<int8_t>(fetch());
    if (!cond) return 2;
    uint16_t newPC = PC + offset;
    int extra = ((PC ^ newPC) & 0xFF00) ? 2 : 1;
    PC = newPC;
    return 2 + extra;
}

// ── Main instruction dispatch ──

int CPU65C02::step()
{
    // Handle WAI (wait for interrupt)
    if (halted) {
        if (m_nmi_pending || m_irq_pending) {
            halted = false;
        } else {
            return 1; // burn 1 cycle waiting
        }
    }

    // Handle NMI
    if (m_nmi_pending) {
        m_nmi_pending = false;
        push16(PC);
        push8(P & ~0x10); // clear B flag
        P |= 0x04; // set I
        P &= ~0x08; // 65C02: clear D on interrupt
        PC = rd16(0xFFFA);
        return 7;
    }

    // Handle IRQ
    if (m_irq_pending && !getI()) {
        m_irq_pending = false;
        push16(PC);
        push8(P & ~0x10);
        P |= 0x04;
        P &= ~0x08; // 65C02: clear D on interrupt
        PC = rd16(0xFFFE);
        return 7;
    }
    m_irq_pending = false;

    uint8_t opcode = fetch();
    bool pageCross = false;
    uint16_t addr;
    uint8_t val, tmp;
    int cycles;

    switch (opcode) {
    // ── BRK ──
    case 0x00:
        PC++;
        push16(PC);
        push8(P | 0x10);
        P |= 0x04;
        P &= ~0x08; // 65C02: clear D
        PC = rd16(0xFFFE);
        return 7;

    // ── ORA ──
    case 0x01: addr = addr_ind_x(); A |= rd(addr); setZN(A); return 6;
    case 0x05: addr = addr_zp();    A |= rd(addr); setZN(A); return 3;
    case 0x09: A |= fetch(); setZN(A); return 2;
    case 0x0D: addr = addr_abs();   A |= rd(addr); setZN(A); return 4;
    case 0x11: addr = addr_ind_y(pageCross); A |= rd(addr); setZN(A); return 5 + pageCross;
    case 0x12: addr = addr_zp_ind(); A |= rd(addr); setZN(A); return 5; // 65C02: ORA (zp)
    case 0x15: addr = addr_zp_x();  A |= rd(addr); setZN(A); return 4;
    case 0x19: addr = addr_abs_y(pageCross); A |= rd(addr); setZN(A); return 4 + pageCross;
    case 0x1D: addr = addr_abs_x(pageCross); A |= rd(addr); setZN(A); return 4 + pageCross;

    // ── ASL ──
    case 0x06: addr = addr_zp();  val = rd(addr); setC(val & 0x80); val <<= 1; wr(addr, val); setZN(val); return 5;
    case 0x0A: setC(A & 0x80); A <<= 1; setZN(A); return 2;
    case 0x0E: addr = addr_abs(); val = rd(addr); setC(val & 0x80); val <<= 1; wr(addr, val); setZN(val); return 6;
    case 0x16: addr = addr_zp_x(); val = rd(addr); setC(val & 0x80); val <<= 1; wr(addr, val); setZN(val); return 6;
    case 0x1E: addr = addr_abs_x(pageCross); val = rd(addr); setC(val & 0x80); val <<= 1; wr(addr, val); setZN(val); return 7;

    // ── BPL / BMI / BVC / BVS / BCC / BCS / BNE / BEQ ──
    case 0x10: return doBranch(!getN());
    case 0x30: return doBranch(getN());
    case 0x50: return doBranch(!getV());
    case 0x70: return doBranch(getV());
    case 0x90: return doBranch(!getC());
    case 0xB0: return doBranch(getC());
    case 0xD0: return doBranch(!getZ());
    case 0xF0: return doBranch(getZ());
    case 0x80: return doBranch(true); // 65C02: BRA (always)

    // ── PHP / PLP / PHA / PLA ──
    case 0x08: push8(P | 0x30); return 3;
    case 0x28: P = (pull8() & ~0x30) | 0x20; return 4;
    case 0x48: push8(A); return 3;
    case 0x68: A = pull8(); setZN(A); return 4;

    // 65C02: PHX / PLX / PHY / PLY
    case 0xDA: push8(X); return 3;
    case 0xFA: X = pull8(); setZN(X); return 4;
    case 0x5A: push8(Y); return 3;
    case 0x7A: Y = pull8(); setZN(Y); return 4;

    // ── CLC / SEC / CLI / SEI / CLV / CLD / SED ──
    case 0x18: P &= ~0x01; return 2;
    case 0x38: P |= 0x01;  return 2;
    case 0x58: P &= ~0x04; return 2;
    case 0x78: P |= 0x04;  return 2;
    case 0xB8: P &= ~0x40; return 2;
    case 0xD8: P &= ~0x08; return 2;
    case 0xF8: P |= 0x08;  return 2;

    // ── AND ──
    case 0x21: addr = addr_ind_x(); A &= rd(addr); setZN(A); return 6;
    case 0x25: addr = addr_zp();    A &= rd(addr); setZN(A); return 3;
    case 0x29: A &= fetch(); setZN(A); return 2;
    case 0x2D: addr = addr_abs();   A &= rd(addr); setZN(A); return 4;
    case 0x31: addr = addr_ind_y(pageCross); A &= rd(addr); setZN(A); return 5 + pageCross;
    case 0x32: addr = addr_zp_ind(); A &= rd(addr); setZN(A); return 5; // 65C02: AND (zp)
    case 0x35: addr = addr_zp_x();  A &= rd(addr); setZN(A); return 4;
    case 0x39: addr = addr_abs_y(pageCross); A &= rd(addr); setZN(A); return 4 + pageCross;
    case 0x3D: addr = addr_abs_x(pageCross); A &= rd(addr); setZN(A); return 4 + pageCross;

    // ── BIT ──
    case 0x24: addr = addr_zp();  val = rd(addr); P = (P & ~0xC2) | (val & 0xC0) | ((A & val) == 0 ? 0x02 : 0); return 3;
    case 0x2C: addr = addr_abs(); val = rd(addr); P = (P & ~0xC2) | (val & 0xC0) | ((A & val) == 0 ? 0x02 : 0); return 4;
    // 65C02: BIT imm, zp,X, abs,X
    case 0x89: val = fetch(); P = (P & ~0x02) | ((A & val) == 0 ? 0x02 : 0); return 2; // BIT # — only Z
    case 0x34: addr = addr_zp_x(); val = rd(addr); P = (P & ~0xC2) | (val & 0xC0) | ((A & val) == 0 ? 0x02 : 0); return 4;
    case 0x3C: addr = addr_abs_x(pageCross); val = rd(addr); P = (P & ~0xC2) | (val & 0xC0) | ((A & val) == 0 ? 0x02 : 0); return 4 + pageCross;

    // ── ROL / ROR ──
    case 0x26: addr = addr_zp();  val = rd(addr); tmp = (val << 1) | (getC() ? 1 : 0); setC(val & 0x80); wr(addr, tmp); setZN(tmp); return 5;
    case 0x2A: tmp = (A << 1) | (getC() ? 1 : 0); setC(A & 0x80); A = tmp; setZN(A); return 2;
    case 0x2E: addr = addr_abs(); val = rd(addr); tmp = (val << 1) | (getC() ? 1 : 0); setC(val & 0x80); wr(addr, tmp); setZN(tmp); return 6;
    case 0x36: addr = addr_zp_x(); val = rd(addr); tmp = (val << 1) | (getC() ? 1 : 0); setC(val & 0x80); wr(addr, tmp); setZN(tmp); return 6;
    case 0x3E: addr = addr_abs_x(pageCross); val = rd(addr); tmp = (val << 1) | (getC() ? 1 : 0); setC(val & 0x80); wr(addr, tmp); setZN(tmp); return 7;

    case 0x66: addr = addr_zp();  val = rd(addr); tmp = (val >> 1) | (getC() ? 0x80 : 0); setC(val & 0x01); wr(addr, tmp); setZN(tmp); return 5;
    case 0x6A: tmp = (A >> 1) | (getC() ? 0x80 : 0); setC(A & 0x01); A = tmp; setZN(A); return 2;
    case 0x6E: addr = addr_abs(); val = rd(addr); tmp = (val >> 1) | (getC() ? 0x80 : 0); setC(val & 0x01); wr(addr, tmp); setZN(tmp); return 6;
    case 0x76: addr = addr_zp_x(); val = rd(addr); tmp = (val >> 1) | (getC() ? 0x80 : 0); setC(val & 0x01); wr(addr, tmp); setZN(tmp); return 6;
    case 0x7E: addr = addr_abs_x(pageCross); val = rd(addr); tmp = (val >> 1) | (getC() ? 0x80 : 0); setC(val & 0x01); wr(addr, tmp); setZN(tmp); return 7;

    // ── EOR ──
    case 0x41: addr = addr_ind_x(); A ^= rd(addr); setZN(A); return 6;
    case 0x45: addr = addr_zp();    A ^= rd(addr); setZN(A); return 3;
    case 0x49: A ^= fetch(); setZN(A); return 2;
    case 0x4D: addr = addr_abs();   A ^= rd(addr); setZN(A); return 4;
    case 0x51: addr = addr_ind_y(pageCross); A ^= rd(addr); setZN(A); return 5 + pageCross;
    case 0x52: addr = addr_zp_ind(); A ^= rd(addr); setZN(A); return 5; // 65C02: EOR (zp)
    case 0x55: addr = addr_zp_x();  A ^= rd(addr); setZN(A); return 4;
    case 0x59: addr = addr_abs_y(pageCross); A ^= rd(addr); setZN(A); return 4 + pageCross;
    case 0x5D: addr = addr_abs_x(pageCross); A ^= rd(addr); setZN(A); return 4 + pageCross;

    // ── ADC ──
    case 0x61: op_adc(rd(addr_ind_x())); return 6;
    case 0x65: op_adc(rd(addr_zp())); return 3;
    case 0x69: op_adc(fetch()); return 2;
    case 0x6D: op_adc(rd(addr_abs())); return 4;
    case 0x71: addr = addr_ind_y(pageCross); op_adc(rd(addr)); return 5 + pageCross;
    case 0x72: op_adc(rd(addr_zp_ind())); return 5; // 65C02: ADC (zp)
    case 0x75: op_adc(rd(addr_zp_x())); return 4;
    case 0x79: addr = addr_abs_y(pageCross); op_adc(rd(addr)); return 4 + pageCross;
    case 0x7D: addr = addr_abs_x(pageCross); op_adc(rd(addr)); return 4 + pageCross;

    // ── SBC ──
    case 0xE1: op_sbc(rd(addr_ind_x())); return 6;
    case 0xE5: op_sbc(rd(addr_zp())); return 3;
    case 0xE9: op_sbc(fetch()); return 2;
    case 0xED: op_sbc(rd(addr_abs())); return 4;
    case 0xF1: addr = addr_ind_y(pageCross); op_sbc(rd(addr)); return 5 + pageCross;
    case 0xF2: op_sbc(rd(addr_zp_ind())); return 5; // 65C02: SBC (zp)
    case 0xF5: op_sbc(rd(addr_zp_x())); return 4;
    case 0xF9: addr = addr_abs_y(pageCross); op_sbc(rd(addr)); return 4 + pageCross;
    case 0xFD: addr = addr_abs_x(pageCross); op_sbc(rd(addr)); return 4 + pageCross;

    // ── STA ──
    case 0x81: wr(addr_ind_x(), A); return 6;
    case 0x85: wr(addr_zp(),    A); return 3;
    case 0x8D: wr(addr_abs(),   A); return 4;
    case 0x91: wr(addr_ind_y(pageCross), A); return 6;
    case 0x92: wr(addr_zp_ind(), A); return 5; // 65C02: STA (zp)
    case 0x95: wr(addr_zp_x(),  A); return 4;
    case 0x99: wr(addr_abs_y(pageCross), A); return 5;
    case 0x9D: wr(addr_abs_x(pageCross), A); return 5;

    // ── STX ──
    case 0x86: wr(addr_zp(),  X); return 3;
    case 0x8E: wr(addr_abs(), X); return 4;
    case 0x96: wr(addr_zp_y(), X); return 4;

    // ── STY ──
    case 0x84: wr(addr_zp(),  Y); return 3;
    case 0x8C: wr(addr_abs(), Y); return 4;
    case 0x94: wr(addr_zp_x(), Y); return 4;

    // 65C02: STZ (store zero)
    case 0x64: wr(addr_zp(),  0); return 3;
    case 0x74: wr(addr_zp_x(), 0); return 4;
    case 0x9C: wr(addr_abs(), 0); return 4;
    case 0x9E: wr(addr_abs_x(pageCross), 0); return 5;

    // ── LDA ──
    case 0xA1: A = rd(addr_ind_x()); setZN(A); return 6;
    case 0xA5: A = rd(addr_zp());    setZN(A); return 3;
    case 0xA9: A = fetch(); setZN(A); return 2;
    case 0xAD: A = rd(addr_abs());   setZN(A); return 4;
    case 0xB1: addr = addr_ind_y(pageCross); A = rd(addr); setZN(A); return 5 + pageCross;
    case 0xB2: A = rd(addr_zp_ind()); setZN(A); return 5; // 65C02: LDA (zp)
    case 0xB5: A = rd(addr_zp_x());  setZN(A); return 4;
    case 0xB9: addr = addr_abs_y(pageCross); A = rd(addr); setZN(A); return 4 + pageCross;
    case 0xBD: addr = addr_abs_x(pageCross); A = rd(addr); setZN(A); return 4 + pageCross;

    // ── LDX ──
    case 0xA2: X = fetch(); setZN(X); return 2;
    case 0xA6: X = rd(addr_zp());   setZN(X); return 3;
    case 0xAE: X = rd(addr_abs());  setZN(X); return 4;
    case 0xB6: X = rd(addr_zp_y()); setZN(X); return 4;
    case 0xBE: addr = addr_abs_y(pageCross); X = rd(addr); setZN(X); return 4 + pageCross;

    // ── LDY ──
    case 0xA0: Y = fetch(); setZN(Y); return 2;
    case 0xA4: Y = rd(addr_zp());   setZN(Y); return 3;
    case 0xAC: Y = rd(addr_abs());  setZN(Y); return 4;
    case 0xB4: Y = rd(addr_zp_x()); setZN(Y); return 4;
    case 0xBC: addr = addr_abs_x(pageCross); Y = rd(addr); setZN(Y); return 4 + pageCross;

    // ── CMP ──
    case 0xC1: op_cmp(A, rd(addr_ind_x())); return 6;
    case 0xC5: op_cmp(A, rd(addr_zp()));    return 3;
    case 0xC9: op_cmp(A, fetch()); return 2;
    case 0xCD: op_cmp(A, rd(addr_abs()));   return 4;
    case 0xD1: addr = addr_ind_y(pageCross); op_cmp(A, rd(addr)); return 5 + pageCross;
    case 0xD2: op_cmp(A, rd(addr_zp_ind())); return 5; // 65C02: CMP (zp)
    case 0xD5: op_cmp(A, rd(addr_zp_x()));  return 4;
    case 0xD9: addr = addr_abs_y(pageCross); op_cmp(A, rd(addr)); return 4 + pageCross;
    case 0xDD: addr = addr_abs_x(pageCross); op_cmp(A, rd(addr)); return 4 + pageCross;

    // ── CPX / CPY ──
    case 0xE0: op_cmp(X, fetch()); return 2;
    case 0xE4: op_cmp(X, rd(addr_zp())); return 3;
    case 0xEC: op_cmp(X, rd(addr_abs())); return 4;
    case 0xC0: op_cmp(Y, fetch()); return 2;
    case 0xC4: op_cmp(Y, rd(addr_zp())); return 3;
    case 0xCC: op_cmp(Y, rd(addr_abs())); return 4;

    // ── INC / DEC (memory) ──
    case 0xC6: addr = addr_zp();  val = rd(addr) - 1; wr(addr, val); setZN(val); return 5;
    case 0xCE: addr = addr_abs(); val = rd(addr) - 1; wr(addr, val); setZN(val); return 6;
    case 0xD6: addr = addr_zp_x(); val = rd(addr) - 1; wr(addr, val); setZN(val); return 6;
    case 0xDE: addr = addr_abs_x(pageCross); val = rd(addr) - 1; wr(addr, val); setZN(val); return 7;

    case 0xE6: addr = addr_zp();  val = rd(addr) + 1; wr(addr, val); setZN(val); return 5;
    case 0xEE: addr = addr_abs(); val = rd(addr) + 1; wr(addr, val); setZN(val); return 6;
    case 0xF6: addr = addr_zp_x(); val = rd(addr) + 1; wr(addr, val); setZN(val); return 6;
    case 0xFE: addr = addr_abs_x(pageCross); val = rd(addr) + 1; wr(addr, val); setZN(val); return 7;

    // ── INC A / DEC A (65C02) ──
    case 0x1A: A++; setZN(A); return 2;
    case 0x3A: A--; setZN(A); return 2;

    // ── INX / DEX / INY / DEY ──
    case 0xE8: X++; setZN(X); return 2;
    case 0xCA: X--; setZN(X); return 2;
    case 0xC8: Y++; setZN(Y); return 2;
    case 0x88: Y--; setZN(Y); return 2;

    // ── TAX / TXA / TAY / TYA / TSX / TXS ──
    case 0xAA: X = A; setZN(X); return 2;
    case 0x8A: A = X; setZN(A); return 2;
    case 0xA8: Y = A; setZN(Y); return 2;
    case 0x98: A = Y; setZN(A); return 2;
    case 0xBA: X = SP; setZN(X); return 2;
    case 0x9A: SP = X; return 2;

    // ── LSR ──
    case 0x46: addr = addr_zp();  val = rd(addr); setC(val & 1); val >>= 1; wr(addr, val); setZN(val); return 5;
    case 0x4A: setC(A & 1); A >>= 1; setZN(A); return 2;
    case 0x4E: addr = addr_abs(); val = rd(addr); setC(val & 1); val >>= 1; wr(addr, val); setZN(val); return 6;
    case 0x56: addr = addr_zp_x(); val = rd(addr); setC(val & 1); val >>= 1; wr(addr, val); setZN(val); return 6;
    case 0x5E: addr = addr_abs_x(pageCross); val = rd(addr); setC(val & 1); val >>= 1; wr(addr, val); setZN(val); return 7;

    // ── JMP ──
    case 0x4C: PC = addr_abs(); return 3;
    case 0x6C: { // JMP (abs) — 65C02 fixes the page boundary bug
        uint16_t ptr = addr_abs();
        PC = rd16(ptr);
        return 6;
    }
    case 0x7C: PC = addr_abs_ind_x(); return 6; // 65C02: JMP (abs,X)

    // ── JSR / RTS / RTI ──
    case 0x20: addr = addr_abs(); push16(PC - 1); PC = addr; return 6;
    case 0x60: PC = pull16() + 1; return 6;
    case 0x40:
        P = (pull8() & ~0x30) | 0x20;
        PC = pull16();
        return 6;

    // ── NOP ──
    case 0xEA: return 2;

    // 65C02: TRB (Test and Reset Bits)
    case 0x04: addr = addr_zp();  val = rd(addr); P = (P & ~0x02) | ((A & val) == 0 ? 0x02 : 0); wr(addr, val & ~A); return 5;
    case 0x0C: addr = addr_abs(); val = rd(addr); P = (P & ~0x02) | ((A & val) == 0 ? 0x02 : 0); wr(addr, val & ~A); return 6;

    // 65C02: TSB (Test and Set Bits)
    case 0x14: addr = addr_zp();  val = rd(addr); P = (P & ~0x02) | ((A & val) == 0 ? 0x02 : 0); wr(addr, val | A); return 5;
    case 0x1C: addr = addr_abs(); val = rd(addr); P = (P & ~0x02) | ((A & val) == 0 ? 0x02 : 0); wr(addr, val | A); return 6;

    // 65C02: WAI (Wait for Interrupt)
    case 0xCB: halted = true; return 3;

    // 65C02: STP (Stop — halt completely, only reset recovers)
    case 0xDB: halted = true; return 3;

    // ── RMBn / SMBn (65C02 Rockwell/WDC) ──
    case 0x07: addr = addr_zp(); wr(addr, rd(addr) & ~0x01); return 5;
    case 0x17: addr = addr_zp(); wr(addr, rd(addr) & ~0x02); return 5;
    case 0x27: addr = addr_zp(); wr(addr, rd(addr) & ~0x04); return 5;
    case 0x37: addr = addr_zp(); wr(addr, rd(addr) & ~0x08); return 5;
    case 0x47: addr = addr_zp(); wr(addr, rd(addr) & ~0x10); return 5;
    case 0x57: addr = addr_zp(); wr(addr, rd(addr) & ~0x20); return 5;
    case 0x67: addr = addr_zp(); wr(addr, rd(addr) & ~0x40); return 5;
    case 0x77: addr = addr_zp(); wr(addr, rd(addr) & ~0x80); return 5;

    case 0x87: addr = addr_zp(); wr(addr, rd(addr) | 0x01); return 5;
    case 0x97: addr = addr_zp(); wr(addr, rd(addr) | 0x02); return 5;
    case 0xA7: addr = addr_zp(); wr(addr, rd(addr) | 0x04); return 5;
    case 0xB7: addr = addr_zp(); wr(addr, rd(addr) | 0x08); return 5;
    case 0xC7: addr = addr_zp(); wr(addr, rd(addr) | 0x10); return 5;
    case 0xD7: addr = addr_zp(); wr(addr, rd(addr) | 0x20); return 5;
    case 0xE7: addr = addr_zp(); wr(addr, rd(addr) | 0x40); return 5;
    case 0xF7: addr = addr_zp(); wr(addr, rd(addr) | 0x80); return 5;

    // ── BBRn / BBSn (65C02 Rockwell/WDC) ──
    // BBR: branch if bit N clear
    case 0x0F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x01)) { PC += off; return 7; } } return 5;
    case 0x1F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x02)) { PC += off; return 7; } } return 5;
    case 0x2F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x04)) { PC += off; return 7; } } return 5;
    case 0x3F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x08)) { PC += off; return 7; } } return 5;
    case 0x4F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x10)) { PC += off; return 7; } } return 5;
    case 0x5F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x20)) { PC += off; return 7; } } return 5;
    case 0x6F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x40)) { PC += off; return 7; } } return 5;
    case 0x7F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (!(val & 0x80)) { PC += off; return 7; } } return 5;
    // BBS: branch if bit N set
    case 0x8F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x01) { PC += off; return 7; } } return 5;
    case 0x9F: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x02) { PC += off; return 7; } } return 5;
    case 0xAF: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x04) { PC += off; return 7; } } return 5;
    case 0xBF: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x08) { PC += off; return 7; } } return 5;
    case 0xCF: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x10) { PC += off; return 7; } } return 5;
    case 0xDF: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x20) { PC += off; return 7; } } return 5;
    case 0xEF: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x40) { PC += off; return 7; } } return 5;
    case 0xFF: addr = addr_zp(); val = rd(addr); { int8_t off = (int8_t)fetch(); if (val & 0x80) { PC += off; return 7; } } return 5;

    // ── All undefined opcodes → NOP (65C02 behavior) ──
    default:
        // Multi-byte NOPs: consume operand bytes based on addressing mode hints
        // Most undefined 65C02 opcodes are 1-byte NOPs
        return 2;
    }
}
