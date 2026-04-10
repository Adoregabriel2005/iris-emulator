#include "CPU6507.h"
#include <QDebug>

CPU6507::CPU6507(ReadFunc r, WriteFunc w)
    : read(r), write(w)
{
}

CPU6507::~CPU6507()
{
}

void CPU6507::reset()
{
    A = X = Y = 0;
    P = 0x24;
    SP = 0xFD;
    PC = rd16(0x1FFC) & ADDR_MASK;
}

void CPU6507::push(uint8_t v)
{
    wr(0x0100 | SP, v);
    SP = (uint8_t)(SP - 1);
}

uint8_t CPU6507::pull()
{
    SP = (uint8_t)(SP + 1);
    return rd(0x0100 | SP);
}

void CPU6507::serialize(QDataStream &ds) const
{
    ds << A << X << Y << P << SP << PC;
}

void CPU6507::deserialize(QDataStream &ds)
{
    ds >> A >> X >> Y >> P >> SP >> PC;
}

// --- Addressing mode helpers ---

uint8_t CPU6507::fetch()
{
    uint8_t v = rd(PC);
    PC = (PC + 1) & ADDR_MASK;
    return v;
}

uint16_t CPU6507::zp_addr()
{
    return uint16_t(fetch());
}

uint16_t CPU6507::zp_x_addr()
{
    return uint16_t((fetch() + X) & 0xFF);
}

uint16_t CPU6507::zp_y_addr()
{
    return uint16_t((fetch() + Y) & 0xFF);
}

uint16_t CPU6507::abs_addr()
{
    uint8_t lo = fetch();
    uint8_t hi = fetch();
    return (uint16_t(hi) << 8 | lo) & ADDR_MASK;
}

uint16_t CPU6507::abs_x_addr(bool &pageCross)
{
    uint8_t lo = fetch();
    uint8_t hi = fetch();
    uint16_t base = (uint16_t(hi) << 8) | lo;
    uint16_t addr = (base + X) & 0xFFFF;
    pageCross = ((base & 0xFF00) != (addr & 0xFF00));
    return addr & ADDR_MASK;
}

uint16_t CPU6507::abs_y_addr(bool &pageCross)
{
    uint8_t lo = fetch();
    uint8_t hi = fetch();
    uint16_t base = (uint16_t(hi) << 8) | lo;
    uint16_t addr = (base + Y) & 0xFFFF;
    pageCross = ((base & 0xFF00) != (addr & 0xFF00));
    return addr & ADDR_MASK;
}

uint16_t CPU6507::ind_addr()
{
    uint8_t lo = fetch();
    uint8_t hi = fetch();
    uint16_t ptr = (uint16_t(hi) << 8) | lo;
    uint8_t l = rd(ptr & ADDR_MASK);
    uint8_t h;
    if ((ptr & 0x00FF) == 0x00FF) {
        h = rd((ptr & 0xFF00) & ADDR_MASK);
    } else {
        h = rd((ptr + 1) & ADDR_MASK);
    }
    return (uint16_t(h) << 8 | l) & ADDR_MASK;
}

uint16_t CPU6507::ind_x_addr()
{
    uint8_t ptr = (fetch() + X) & 0xFF;
    uint8_t lo = rd(ptr & 0xFF);
    uint8_t hi = rd((ptr + 1) & 0xFF);
    return (uint16_t(hi) << 8 | lo) & ADDR_MASK;
}

uint16_t CPU6507::ind_y_addr(bool &pageCross)
{
    uint8_t ptr = fetch();
    uint8_t lo = rd(ptr & 0xFF);
    uint8_t hi = rd((ptr + 1) & 0xFF);
    uint16_t base = (uint16_t(hi) << 8) | lo;
    uint16_t addr = (base + Y) & 0xFFFF;
    pageCross = ((base & 0xFF00) != (addr & 0xFF00));
    return addr & ADDR_MASK;
}

// --- Main step function ---

int CPU6507::step()
{
    m_busAccess = 0; // reset bus access counter for cycle-accurate TIA sync
    uint8_t op = fetch();

    // Helper lambdas before switch to avoid MSVC C2360
    auto op_adc = [&](uint8_t v) {
        uint8_t c = P & 0x01;
        if (P & 0x08) { // BCD decimal mode
            uint16_t al = (A & 0x0F) + (v & 0x0F) + c;
            uint16_t ah = (uint16_t)(A >> 4) + (v >> 4);
            // Z flag based on binary result (NMOS 6502 behavior)
            uint8_t binr = (uint8_t)((uint16_t)A + v + c);
            if (binr == 0) P |= 0x02; else P &= ~0x02;
            if (al > 9) { al += 6; ah++; }
            // N and V flags from intermediate (after low adjust, before high adjust)
            uint8_t interim = (uint8_t)((ah << 4) | (al & 0x0F));
            P = (P & ~0x80) | (interim & 0x80);
            if ((~(A ^ v) & (A ^ interim)) & 0x80) P |= 0x40; else P &= ~0x40;
            if (ah > 9) ah += 6;
            if (ah > 15) P |= 0x01; else P &= ~0x01;
            A = ((ah & 0x0F) << 4) | (al & 0x0F);
        } else {
            uint16_t res = uint16_t(A) + uint16_t(v) + c;
            if (res & 0x100) P |= 0x01; else P &= ~0x01;
            uint8_t r = uint8_t(res & 0xFF);
            if ((~(A ^ v) & (A ^ r)) & 0x80) P |= 0x40; else P &= ~0x40;
            A = r; setZN(A);
        }
    };

    auto op_sbc = [&](uint8_t v) {
        uint8_t c = P & 0x01;
        // Binary result for flags (NMOS 6502: N, V, Z, C always from binary)
        uint16_t binval = uint16_t(v) ^ 0xFF;
        uint16_t binres = uint16_t(A) + binval + c;
        uint8_t binr = uint8_t(binres & 0xFF);
        if (P & 0x08) { // BCD decimal mode
            int al = (int)(A & 0x0F) - (v & 0x0F) - (1 - c);
            int ah = (int)(A >> 4) - (v >> 4);
            if (al < 0) { al = (al - 6) & 0x0F; ah--; }
            if (ah < 0) { ah = (ah - 6) & 0x0F; }
            // C, N, V, Z from binary result (NMOS behavior)
            if (binres & 0x100) P |= 0x01; else P &= ~0x01;
            if ((~(A ^ binval) & (A ^ binr)) & 0x80) P |= 0x40; else P &= ~0x40;
            if (binr == 0) P |= 0x02; else P &= ~0x02;
            P = (P & ~0x80) | (binr & 0x80);
            A = ((ah & 0x0F) << 4) | (al & 0x0F);
        } else {
            if (binres & 0x100) P |= 0x01; else P &= ~0x01;
            if ((~(A ^ binval) & (A ^ binr)) & 0x80) P |= 0x40; else P &= ~0x40;
            A = binr; setZN(A);
        }
    };

    auto op_cmp = [&](uint8_t reg, uint8_t v) {
        uint16_t r = uint16_t(reg) - uint16_t(v);
        if ((r & 0x100) == 0) P |= 0x01; else P &= ~0x01;
        uint8_t res = uint8_t(r & 0xFF);
        if (res == 0) P |= 0x02; else P &= ~0x02;
        P = (P & ~0x80) | (res & 0x80);
    };

    auto do_branch = [&](bool cond) -> int {
        int8_t off = (int8_t)fetch();
        if (!cond) return 2;
        uint16_t oldpc = PC;
        PC = (PC + off) & ADDR_MASK;
        int cycles = 3;
        if ((oldpc & 0xFF00) != (PC & 0xFF00)) cycles += 1;
        return cycles;
    };

    // ── Illegal opcode helpers ──
    auto op_slo = [&](uint16_t addr) { // ASL memory + ORA result into A
        uint8_t v = rd(addr);
        uint8_t c = (v >> 7) & 1;
        v <<= 1;
        wr(addr, v);
        if (c) P |= 0x01; else P &= ~0x01;
        A |= v;
        setZN(A);
    };
    auto op_rla = [&](uint16_t addr) { // ROL memory + AND result into A
        uint8_t v = rd(addr);
        uint8_t c = (v >> 7) & 1;
        uint8_t oc = P & 0x01;
        v = (v << 1) | oc;
        wr(addr, v);
        if (c) P |= 0x01; else P &= ~0x01;
        A &= v;
        setZN(A);
    };
    auto op_sre = [&](uint16_t addr) { // LSR memory + EOR result into A
        uint8_t v = rd(addr);
        uint8_t c = v & 1;
        v >>= 1;
        wr(addr, v);
        if (c) P |= 0x01; else P &= ~0x01;
        A ^= v;
        setZN(A);
    };
    auto op_rra = [&](uint16_t addr) { // ROR memory + ADC result into A
        uint8_t v = rd(addr);
        uint8_t c = v & 1;
        uint8_t oc = (P & 0x01) << 7;
        v = (v >> 1) | oc;
        wr(addr, v);
        if (c) P |= 0x01; else P &= ~0x01;
        op_adc(v);
    };
    auto op_dcp = [&](uint16_t addr) { // DEC memory + CMP with A
        uint8_t v = rd(addr) - 1;
        wr(addr, v);
        op_cmp(A, v);
    };
    auto op_isb = [&](uint16_t addr) { // INC memory + SBC from A
        uint8_t v = rd(addr) + 1;
        wr(addr, v);
        op_sbc(v);
    };
    auto op_lax = [&](uint16_t addr) { // LDA + LDX from memory
        A = X = rd(addr);
        setZN(A);
    };
    auto op_sax = [&](uint16_t addr) { // Store A AND X to memory
        wr(addr, A & X);
    };

    switch (op) {
    // --- LDA ---
    case 0xA9: { uint8_t v = fetch(); A = v; setZN(A); return 2; }
    case 0xA5: { uint16_t a = zp_addr(); A = rd(a); setZN(A); return 3; }
    case 0xB5: { uint16_t a = zp_x_addr(); A = rd(a); setZN(A); return 4; }
    case 0xAD: { uint16_t a = abs_addr(); A = rd(a); setZN(A); return 4; }
    case 0xBD: { bool pc=false; uint16_t a = abs_x_addr(pc); A = rd(a); setZN(A); return 4+(pc?1:0); }
    case 0xB9: { bool pc=false; uint16_t a = abs_y_addr(pc); A = rd(a); setZN(A); return 4+(pc?1:0); }
    case 0xA1: { uint16_t a = ind_x_addr(); A = rd(a); setZN(A); return 6; }
    case 0xB1: { bool pc=false; uint16_t a = ind_y_addr(pc); A = rd(a); setZN(A); return 5+(pc?1:0); }

    // --- LDX ---
    case 0xA2: { X = fetch(); setZN(X); return 2; }
    case 0xA6: { uint16_t a = zp_addr(); X = rd(a); setZN(X); return 3; }
    case 0xB6: { uint16_t a = zp_y_addr(); X = rd(a); setZN(X); return 4; }
    case 0xAE: { uint16_t a = abs_addr(); X = rd(a); setZN(X); return 4; }
    case 0xBE: { bool pc=false; uint16_t a = abs_y_addr(pc); X = rd(a); setZN(X); return 4+(pc?1:0); }

    // --- LDY ---
    case 0xA0: { Y = fetch(); setZN(Y); return 2; }
    case 0xA4: { uint16_t a = zp_addr(); Y = rd(a); setZN(Y); return 3; }
    case 0xB4: { uint16_t a = zp_x_addr(); Y = rd(a); setZN(Y); return 4; }
    case 0xAC: { uint16_t a = abs_addr(); Y = rd(a); setZN(Y); return 4; }
    case 0xBC: { bool pc=false; uint16_t a = abs_x_addr(pc); Y = rd(a); setZN(Y); return 4+(pc?1:0); }

    // --- STA ---
    case 0x85: { uint16_t a = zp_addr(); wr(a, A); return 3; }
    case 0x95: { uint16_t a = zp_x_addr(); wr(a, A); return 4; }
    case 0x8D: { uint16_t a = abs_addr(); wr(a, A); return 4; }
    case 0x9D: { bool pc=false; uint16_t a = abs_x_addr(pc); wr(a, A); return 5; }
    case 0x99: { bool pc=false; uint16_t a = abs_y_addr(pc); wr(a, A); return 5; }
    case 0x81: { uint16_t a = ind_x_addr(); wr(a, A); return 6; }
    case 0x91: { bool pc=false; uint16_t a = ind_y_addr(pc); wr(a, A); return 6; }

    // --- STX/STY ---
    case 0x86: { uint16_t a = zp_addr(); wr(a, X); return 3; }
    case 0x96: { uint16_t a = zp_y_addr(); wr(a, X); return 4; }
    case 0x8E: { uint16_t a = abs_addr(); wr(a, X); return 4; }
    case 0x84: { uint16_t a = zp_addr(); wr(a, Y); return 3; }
    case 0x94: { uint16_t a = zp_x_addr(); wr(a, Y); return 4; }
    case 0x8C: { uint16_t a = abs_addr(); wr(a, Y); return 4; }

    // --- TAX/TAY/TXA/TYA/TSX/TXS ---
    case 0xAA: { X = A; setZN(X); return 2; }
    case 0xA8: { Y = A; setZN(Y); return 2; }
    case 0x8A: { A = X; setZN(A); return 2; }
    case 0x98: { A = Y; setZN(A); return 2; }
    case 0xBA: { X = SP; setZN(X); return 2; }
    case 0x9A: { SP = X; return 2; }

    // --- Stack ops ---
    case 0x48: { push(A); return 3; }
    case 0x68: { A = pull(); setZN(A); return 4; }
    case 0x08: { push(P | 0x10); return 3; }
    case 0x28: { P = (pull() & ~0x10) | 0x20; return 4; }

    // --- ADC ---
    case 0x69: { op_adc(fetch()); return 2; }
    case 0x65: { uint16_t a = zp_addr(); op_adc(rd(a)); return 3; }
    case 0x75: { uint16_t a = zp_x_addr(); op_adc(rd(a)); return 4; }
    case 0x6D: { uint16_t a = abs_addr(); op_adc(rd(a)); return 4; }
    case 0x7D: { bool pc=false; uint16_t a = abs_x_addr(pc); op_adc(rd(a)); return 4+(pc?1:0); }
    case 0x79: { bool pc=false; uint16_t a = abs_y_addr(pc); op_adc(rd(a)); return 4+(pc?1:0); }
    case 0x61: { uint16_t a = ind_x_addr(); op_adc(rd(a)); return 6; }
    case 0x71: { bool pc=false; uint16_t a = ind_y_addr(pc); op_adc(rd(a)); return 5+(pc?1:0); }

    // --- SBC ---
    case 0xE9: { op_sbc(fetch()); return 2; }
    case 0xE5: { uint16_t a = zp_addr(); op_sbc(rd(a)); return 3; }
    case 0xF5: { uint16_t a = zp_x_addr(); op_sbc(rd(a)); return 4; }
    case 0xED: { uint16_t a = abs_addr(); op_sbc(rd(a)); return 4; }
    case 0xFD: { bool pc=false; uint16_t a = abs_x_addr(pc); op_sbc(rd(a)); return 4+(pc?1:0); }
    case 0xF9: { bool pc=false; uint16_t a = abs_y_addr(pc); op_sbc(rd(a)); return 4+(pc?1:0); }
    case 0xE1: { uint16_t a = ind_x_addr(); op_sbc(rd(a)); return 6; }
    case 0xF1: { bool pc=false; uint16_t a = ind_y_addr(pc); op_sbc(rd(a)); return 5+(pc?1:0); }

    // --- AND ---
    case 0x29: { A &= fetch(); setZN(A); return 2; }
    case 0x25: { uint16_t a = zp_addr(); A &= rd(a); setZN(A); return 3; }
    case 0x35: { uint16_t a = zp_x_addr(); A &= rd(a); setZN(A); return 4; }
    case 0x2D: { uint16_t a = abs_addr(); A &= rd(a); setZN(A); return 4; }
    case 0x3D: { bool pc=false; uint16_t a = abs_x_addr(pc); A &= rd(a); setZN(A); return 4+(pc?1:0); }
    case 0x39: { bool pc=false; uint16_t a = abs_y_addr(pc); A &= rd(a); setZN(A); return 4+(pc?1:0); }
    case 0x21: { uint16_t a = ind_x_addr(); A &= rd(a); setZN(A); return 6; }
    case 0x31: { bool pc=false; uint16_t a = ind_y_addr(pc); A &= rd(a); setZN(A); return 5+(pc?1:0); }

    // --- ORA ---
    case 0x09: { A |= fetch(); setZN(A); return 2; }
    case 0x05: { uint16_t a = zp_addr(); A |= rd(a); setZN(A); return 3; }
    case 0x15: { uint16_t a = zp_x_addr(); A |= rd(a); setZN(A); return 4; }
    case 0x0D: { uint16_t a = abs_addr(); A |= rd(a); setZN(A); return 4; }
    case 0x1D: { bool pc=false; uint16_t a = abs_x_addr(pc); A |= rd(a); setZN(A); return 4+(pc?1:0); }
    case 0x19: { bool pc=false; uint16_t a = abs_y_addr(pc); A |= rd(a); setZN(A); return 4+(pc?1:0); }
    case 0x01: { uint16_t a = ind_x_addr(); A |= rd(a); setZN(A); return 6; }
    case 0x11: { bool pc=false; uint16_t a = ind_y_addr(pc); A |= rd(a); setZN(A); return 5+(pc?1:0); }

    // --- EOR ---
    case 0x49: { A ^= fetch(); setZN(A); return 2; }
    case 0x45: { uint16_t a = zp_addr(); A ^= rd(a); setZN(A); return 3; }
    case 0x55: { uint16_t a = zp_x_addr(); A ^= rd(a); setZN(A); return 4; }
    case 0x4D: { uint16_t a = abs_addr(); A ^= rd(a); setZN(A); return 4; }
    case 0x5D: { bool pc=false; uint16_t a = abs_x_addr(pc); A ^= rd(a); setZN(A); return 4+(pc?1:0); }
    case 0x59: { bool pc=false; uint16_t a = abs_y_addr(pc); A ^= rd(a); setZN(A); return 4+(pc?1:0); }
    case 0x41: { uint16_t a = ind_x_addr(); A ^= rd(a); setZN(A); return 6; }
    case 0x51: { bool pc=false; uint16_t a = ind_y_addr(pc); A ^= rd(a); setZN(A); return 5+(pc?1:0); }

    // --- CMP ---
    case 0xC9: { op_cmp(A, fetch()); return 2; }
    case 0xC5: { uint16_t a = zp_addr(); op_cmp(A, rd(a)); return 3; }
    case 0xD5: { uint16_t a = zp_x_addr(); op_cmp(A, rd(a)); return 4; }
    case 0xCD: { uint16_t a = abs_addr(); op_cmp(A, rd(a)); return 4; }
    case 0xDD: { bool pc=false; uint16_t a = abs_x_addr(pc); op_cmp(A, rd(a)); return 4+(pc?1:0); }
    case 0xD9: { bool pc=false; uint16_t a = abs_y_addr(pc); op_cmp(A, rd(a)); return 4+(pc?1:0); }
    case 0xC1: { uint16_t a = ind_x_addr(); op_cmp(A, rd(a)); return 6; }
    case 0xD1: { bool pc=false; uint16_t a = ind_y_addr(pc); op_cmp(A, rd(a)); return 5+(pc?1:0); }

    // --- CPX ---
    case 0xE0: { op_cmp(X, fetch()); return 2; }
    case 0xE4: { uint16_t a = zp_addr(); op_cmp(X, rd(a)); return 3; }
    case 0xEC: { uint16_t a = abs_addr(); op_cmp(X, rd(a)); return 4; }

    // --- CPY ---
    case 0xC0: { op_cmp(Y, fetch()); return 2; }
    case 0xC4: { uint16_t a = zp_addr(); op_cmp(Y, rd(a)); return 3; }
    case 0xCC: { uint16_t a = abs_addr(); op_cmp(Y, rd(a)); return 4; }

    // --- INC ---
    case 0xE6: { uint16_t a = zp_addr(); uint8_t v = rd(a)+1; wr(a,v); setZN(v); return 5; }
    case 0xF6: { uint16_t a = zp_x_addr(); uint8_t v = rd(a)+1; wr(a,v); setZN(v); return 6; }
    case 0xEE: { uint16_t a = abs_addr(); uint8_t v = rd(a)+1; wr(a,v); setZN(v); return 6; }
    case 0xFE: { bool pc=false; uint16_t a = abs_x_addr(pc); uint8_t v = rd(a)+1; wr(a,v); setZN(v); return 7; }

    // --- DEC ---
    case 0xC6: { uint16_t a = zp_addr(); uint8_t v = rd(a)-1; wr(a,v); setZN(v); return 5; }
    case 0xD6: { uint16_t a = zp_x_addr(); uint8_t v = rd(a)-1; wr(a,v); setZN(v); return 6; }
    case 0xCE: { uint16_t a = abs_addr(); uint8_t v = rd(a)-1; wr(a,v); setZN(v); return 6; }
    case 0xDE: { bool pc=false; uint16_t a = abs_x_addr(pc); uint8_t v = rd(a)-1; wr(a,v); setZN(v); return 7; }

    // --- INX/DEX/INY/DEY ---
    case 0xE8: { X++; setZN(X); return 2; }
    case 0xCA: { X--; setZN(X); return 2; }
    case 0xC8: { Y++; setZN(Y); return 2; }
    case 0x88: { Y--; setZN(Y); return 2; }

    // --- ASL accumulator ---
    case 0x0A: { uint8_t c=(A>>7)&1; A<<=1; if(c) P|=0x01; else P&=~0x01; setZN(A); return 2; }
    case 0x06: { uint16_t a=zp_addr(); uint8_t v=rd(a); uint8_t c=(v>>7)&1; v<<=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 5; }
    case 0x16: { uint16_t a=zp_x_addr(); uint8_t v=rd(a); uint8_t c=(v>>7)&1; v<<=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x0E: { uint16_t a=abs_addr(); uint8_t v=rd(a); uint8_t c=(v>>7)&1; v<<=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x1E: { bool pc=false; uint16_t a=abs_x_addr(pc); uint8_t v=rd(a); uint8_t c=(v>>7)&1; v<<=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 7; }

    // --- LSR accumulator ---
    case 0x4A: { uint8_t c=A&1; A>>=1; if(c) P|=0x01; else P&=~0x01; setZN(A); return 2; }
    case 0x46: { uint16_t a=zp_addr(); uint8_t v=rd(a); uint8_t c=v&1; v>>=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 5; }
    case 0x56: { uint16_t a=zp_x_addr(); uint8_t v=rd(a); uint8_t c=v&1; v>>=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x4E: { uint16_t a=abs_addr(); uint8_t v=rd(a); uint8_t c=v&1; v>>=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x5E: { bool pc=false; uint16_t a=abs_x_addr(pc); uint8_t v=rd(a); uint8_t c=v&1; v>>=1; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 7; }

    // --- ROL accumulator ---
    case 0x2A: { uint8_t c=(A>>7)&1; uint8_t oc=P&0x01; A=(A<<1)|oc; if(c) P|=0x01; else P&=~0x01; setZN(A); return 2; }
    case 0x26: { uint16_t a=zp_addr(); uint8_t v=rd(a); uint8_t c=(v>>7)&1; uint8_t oc=P&0x01; v=(v<<1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 5; }
    case 0x36: { uint16_t a=zp_x_addr(); uint8_t v=rd(a); uint8_t c=(v>>7)&1; uint8_t oc=P&0x01; v=(v<<1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x2E: { uint16_t a=abs_addr(); uint8_t v=rd(a); uint8_t c=(v>>7)&1; uint8_t oc=P&0x01; v=(v<<1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x3E: { bool pc=false; uint16_t a=abs_x_addr(pc); uint8_t v=rd(a); uint8_t c=(v>>7)&1; uint8_t oc=P&0x01; v=(v<<1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 7; }

    // --- ROR accumulator ---
    case 0x6A: { uint8_t c=A&1; uint8_t oc=(P&0x01)<<7; A=(A>>1)|oc; if(c) P|=0x01; else P&=~0x01; setZN(A); return 2; }
    case 0x66: { uint16_t a=zp_addr(); uint8_t v=rd(a); uint8_t c=v&1; uint8_t oc=(P&0x01)<<7; v=(v>>1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 5; }
    case 0x76: { uint16_t a=zp_x_addr(); uint8_t v=rd(a); uint8_t c=v&1; uint8_t oc=(P&0x01)<<7; v=(v>>1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x6E: { uint16_t a=abs_addr(); uint8_t v=rd(a); uint8_t c=v&1; uint8_t oc=(P&0x01)<<7; v=(v>>1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 6; }
    case 0x7E: { bool pc=false; uint16_t a=abs_x_addr(pc); uint8_t v=rd(a); uint8_t c=v&1; uint8_t oc=(P&0x01)<<7; v=(v>>1)|oc; wr(a,v); if(c) P|=0x01; else P&=~0x01; setZN(v); return 7; }

    // --- BIT ---
    case 0x24: { uint16_t a=zp_addr(); uint8_t v=rd(a); uint8_t r=A&v; if(r==0) P|=0x02; else P&=~0x02; P=(P&~0xC0)|(v&0xC0); return 3; }
    case 0x2C: { uint16_t a=abs_addr(); uint8_t v=rd(a); uint8_t r=A&v; if(r==0) P|=0x02; else P&=~0x02; P=(P&~0xC0)|(v&0xC0); return 4; }

    // --- Branches ---
    case 0x10: return do_branch((P & 0x80) == 0); // BPL
    case 0x30: return do_branch((P & 0x80) != 0); // BMI
    case 0x50: return do_branch((P & 0x40) == 0); // BVC
    case 0x70: return do_branch((P & 0x40) != 0); // BVS
    case 0x90: return do_branch((P & 0x01) == 0); // BCC
    case 0xB0: return do_branch((P & 0x01) != 0); // BCS
    case 0xD0: return do_branch((P & 0x02) == 0); // BNE
    case 0xF0: return do_branch((P & 0x02) != 0); // BEQ

    // --- Flags ---
    case 0x18: { P &= ~0x01; return 2; } // CLC
    case 0x38: { P |= 0x01; return 2; }  // SEC
    case 0x58: { P &= ~0x04; return 2; } // CLI
    case 0x78: { P |= 0x04; return 2; }  // SEI
    case 0xD8: { P &= ~0x08; return 2; } // CLD
    case 0xF8: { P |= 0x08; return 2; }  // SED
    case 0xB8: { P &= ~0x40; return 2; } // CLV

    // --- JSR/RTS/JMP/BRK/RTI ---
    case 0x20: { // JSR
        uint16_t addr = rd16(PC);
        PC = (PC + 2) & ADDR_MASK;
        uint16_t ret = (PC - 1) & ADDR_MASK;
        push((ret >> 8) & 0xFF);
        push(ret & 0xFF);
        PC = addr & ADDR_MASK;
        return 6;
    }
    case 0x60: { // RTS
        uint8_t lo = pull(); uint8_t hi = pull();
        PC = ((hi << 8) | lo);
        PC = (PC + 1) & ADDR_MASK;
        return 6;
    }
    case 0x4C: { // JMP abs
        uint16_t addr = rd16(PC);
        PC = addr & ADDR_MASK;
        return 3;
    }
    case 0x6C: { // JMP (ind)
        uint16_t addr = ind_addr();
        PC = addr & ADDR_MASK;
        return 5;
    }
    case 0x00: { // BRK
        uint16_t ret = (PC + 1) & ADDR_MASK; // skip padding byte
        push((ret >> 8) & 0xFF);
        push(ret & 0xFF);
        push(P | 0x10);
        P |= 0x04;
        PC = rd16(0x1FFE) & ADDR_MASK;
        return 7;
    }
    case 0x40: { // RTI
        P = (pull() & ~0x10) | 0x20;
        uint8_t lo = pull(); uint8_t hi = pull();
        PC = ((hi << 8) | lo) & ADDR_MASK;
        return 6;
    }

    case 0xEA: return 2; // NOP

    // ═══════════════════════════════════════════════════════════════════
    // Illegal / undocumented opcodes (used by many Atari 2600 games)
    // ═══════════════════════════════════════════════════════════════════

    // --- NOP variants (various addressing modes, consume bytes/cycles) ---
    case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA:
        return 2; // implied NOP (1 byte, 2 cycles)
    case 0x04: case 0x44: case 0x64:
        { fetch(); return 3; } // zero-page NOP (2 bytes, 3 cycles)
    case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4:
        { fetch(); return 4; } // zero-page,X NOP (2 bytes, 4 cycles)
    case 0x0C:
        { abs_addr(); return 4; } // absolute NOP (3 bytes, 4 cycles)
    case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC:
        { bool pc=false; abs_x_addr(pc); return 4+(pc?1:0); } // absolute,X NOP (3 bytes, 4+ cycles)
    case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2:
        { fetch(); return 2; } // immediate NOP (2 bytes, 2 cycles)

    // --- SLO (ASL + ORA) ---
    case 0x03: { uint16_t a = ind_x_addr(); op_slo(a); return 8; }
    case 0x07: { uint16_t a = zp_addr();    op_slo(a); return 5; }
    case 0x0F: { uint16_t a = abs_addr();   op_slo(a); return 6; }
    case 0x13: { bool pc=false; uint16_t a = ind_y_addr(pc); op_slo(a); return 8; }
    case 0x17: { uint16_t a = zp_x_addr();  op_slo(a); return 6; }
    case 0x1B: { bool pc=false; uint16_t a = abs_y_addr(pc); op_slo(a); return 7; }
    case 0x1F: { bool pc=false; uint16_t a = abs_x_addr(pc); op_slo(a); return 7; }

    // --- RLA (ROL + AND) ---
    case 0x23: { uint16_t a = ind_x_addr(); op_rla(a); return 8; }
    case 0x27: { uint16_t a = zp_addr();    op_rla(a); return 5; }
    case 0x2F: { uint16_t a = abs_addr();   op_rla(a); return 6; }
    case 0x33: { bool pc=false; uint16_t a = ind_y_addr(pc); op_rla(a); return 8; }
    case 0x37: { uint16_t a = zp_x_addr();  op_rla(a); return 6; }
    case 0x3B: { bool pc=false; uint16_t a = abs_y_addr(pc); op_rla(a); return 7; }
    case 0x3F: { bool pc=false; uint16_t a = abs_x_addr(pc); op_rla(a); return 7; }

    // --- SRE (LSR + EOR) ---
    case 0x43: { uint16_t a = ind_x_addr(); op_sre(a); return 8; }
    case 0x47: { uint16_t a = zp_addr();    op_sre(a); return 5; }
    case 0x4F: { uint16_t a = abs_addr();   op_sre(a); return 6; }
    case 0x53: { bool pc=false; uint16_t a = ind_y_addr(pc); op_sre(a); return 8; }
    case 0x57: { uint16_t a = zp_x_addr();  op_sre(a); return 6; }
    case 0x5B: { bool pc=false; uint16_t a = abs_y_addr(pc); op_sre(a); return 7; }
    case 0x5F: { bool pc=false; uint16_t a = abs_x_addr(pc); op_sre(a); return 7; }

    // --- RRA (ROR + ADC) ---
    case 0x63: { uint16_t a = ind_x_addr(); op_rra(a); return 8; }
    case 0x67: { uint16_t a = zp_addr();    op_rra(a); return 5; }
    case 0x6F: { uint16_t a = abs_addr();   op_rra(a); return 6; }
    case 0x73: { bool pc=false; uint16_t a = ind_y_addr(pc); op_rra(a); return 8; }
    case 0x77: { uint16_t a = zp_x_addr();  op_rra(a); return 6; }
    case 0x7B: { bool pc=false; uint16_t a = abs_y_addr(pc); op_rra(a); return 7; }
    case 0x7F: { bool pc=false; uint16_t a = abs_x_addr(pc); op_rra(a); return 7; }

    // --- DCP (DEC + CMP) ---
    case 0xC3: { uint16_t a = ind_x_addr(); op_dcp(a); return 8; }
    case 0xC7: { uint16_t a = zp_addr();    op_dcp(a); return 5; }
    case 0xCF: { uint16_t a = abs_addr();   op_dcp(a); return 6; }
    case 0xD3: { bool pc=false; uint16_t a = ind_y_addr(pc); op_dcp(a); return 8; }
    case 0xD7: { uint16_t a = zp_x_addr();  op_dcp(a); return 6; }
    case 0xDB: { bool pc=false; uint16_t a = abs_y_addr(pc); op_dcp(a); return 7; }
    case 0xDF: { bool pc=false; uint16_t a = abs_x_addr(pc); op_dcp(a); return 7; }

    // --- ISB / ISC (INC + SBC) ---
    case 0xE3: { uint16_t a = ind_x_addr(); op_isb(a); return 8; }
    case 0xE7: { uint16_t a = zp_addr();    op_isb(a); return 5; }
    case 0xEF: { uint16_t a = abs_addr();   op_isb(a); return 6; }
    case 0xF3: { bool pc=false; uint16_t a = ind_y_addr(pc); op_isb(a); return 8; }
    case 0xF7: { uint16_t a = zp_x_addr();  op_isb(a); return 6; }
    case 0xFB: { bool pc=false; uint16_t a = abs_y_addr(pc); op_isb(a); return 7; }
    case 0xFF: { bool pc=false; uint16_t a = abs_x_addr(pc); op_isb(a); return 7; }

    // --- LAX (LDA + LDX) ---
    case 0xA3: { uint16_t a = ind_x_addr(); op_lax(a); return 6; }
    case 0xA7: { uint16_t a = zp_addr();    op_lax(a); return 3; }
    case 0xAF: { uint16_t a = abs_addr();   op_lax(a); return 4; }
    case 0xB3: { bool pc=false; uint16_t a = ind_y_addr(pc); op_lax(a); return 5+(pc?1:0); }
    case 0xB7: { uint16_t a = zp_y_addr();  op_lax(a); return 4; }
    case 0xBF: { bool pc=false; uint16_t a = abs_y_addr(pc); op_lax(a); return 4+(pc?1:0); }

    // --- SAX (store A AND X) ---
    case 0x83: { uint16_t a = ind_x_addr(); op_sax(a); return 6; }
    case 0x87: { uint16_t a = zp_addr();    op_sax(a); return 3; }
    case 0x8F: { uint16_t a = abs_addr();   op_sax(a); return 4; }
    case 0x97: { uint16_t a = zp_y_addr();  op_sax(a); return 4; }

    // --- ANC (AND + carry from bit 7) ---
    case 0x0B: case 0x2B: {
        A &= fetch();
        setZN(A);
        if (A & 0x80) P |= 0x01; else P &= ~0x01;
        return 2;
    }

    // --- ALR / ASR (AND + LSR) ---
    case 0x4B: {
        A &= fetch();
        uint8_t c = A & 1;
        A >>= 1;
        if (c) P |= 0x01; else P &= ~0x01;
        setZN(A);
        return 2;
    }

    // --- ARR (AND + ROR with special flag handling) ---
    case 0x6B: {
        A &= fetch();
        A = (A >> 1) | ((P & 0x01) << 7);
        setZN(A);
        if (A & 0x40) P |= 0x01; else P &= ~0x01;
        if (((A >> 6) ^ (A >> 5)) & 1) P |= 0x40; else P &= ~0x40;
        return 2;
    }

    // --- AXS / SBX ((A AND X) - operand → X), no borrow ---
    case 0xCB: {
        uint8_t v = fetch();
        uint8_t ax = A & X;
        uint16_t r = uint16_t(ax) - uint16_t(v);
        X = uint8_t(r & 0xFF);
        if ((r & 0x100) == 0) P |= 0x01; else P &= ~0x01;
        setZN(X);
        return 2;
    }

    // --- SBC illegal mirror (identical to 0xE9) ---
    case 0xEB: { op_sbc(fetch()); return 2; }

    // --- KIL / JAM (halt CPU — loop on self) ---
    case 0x02: case 0x12: case 0x22: case 0x32:
    case 0x42: case 0x52: case 0x62: case 0x72:
    case 0x92: case 0xB2: case 0xD2: case 0xF2:
        PC = (PC - 1) & ADDR_MASK;
        return 2;

    default:
        qDebug() << "Unimplemented opcode:" << Qt::hex << int(op) << "at PC=" << Qt::hex << PC;
        return 2;
    }
}
