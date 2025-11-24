#include "cpu.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstring>

void CPU::reset(uint16_t pc) {
    A=B=0;
    PC=pc;
    SP=0x7FFF;
    F=0;
    halted=false;
    mem.fill(0);
}

void CPU::load_binary(const std::vector<uint8_t>& bin, uint16_t addr) {
    if (addr + bin.size() > mem.size()) throw std::runtime_error("binary too large");
    std::memcpy(mem.data()+addr, bin.data(), bin.size());
    PC = addr;
}

void CPU::dump(uint16_t addr, size_t len) {
    for (size_t i=0;i<len;i++) {
        if (i%16==0) std::cout << std::hex << std::setw(4) << std::setfill('0') << (addr+i) << ": ";
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)mem[addr+i] << " ";
        if (i%16==15) std::cout << "\n";
    }
    if (len%16) std::cout << "\n";
}

uint8_t CPU::fetch8() { return mem[PC++]; }
uint16_t CPU::fetch16() {
    uint8_t lo = fetch8();
    uint8_t hi = fetch8();
    return (uint16_t)lo | ((uint16_t)hi<<8);
}

void CPU::setZN(uint8_t v) {
    if (v==0) F |= ZERO; else F &= ~ZERO;
    if (v & 0x80) F |= NEG; else F &= ~NEG;
}

void CPU::setAddFlags(uint16_t a, uint16_t b, uint16_t r) {
    if (r & 0x100) F |= CARRY; else F &= ~CARRY;
    uint8_t rv = (uint8_t)r;
    setZN(rv);
    // overflow: (~(a^b) & (a^r)) & 0x80
    uint8_t ov = (~((uint8_t)a ^ (uint8_t)b) & ((uint8_t)a ^ rv)) & 0x80;
    if (ov) F |= OVF; else F &= ~OVF;
}

void CPU::setSubFlags(uint16_t a, uint16_t b, uint16_t r) {
    // In subtraction, carry=NOT borrow => r doesn't underflow => a>=b
    if (a >= b) F |= CARRY; else F &= ~CARRY;
    uint8_t rv = (uint8_t)r;
    setZN(rv);
    // overflow: ((a^b) & (a^r)) & 0x80
    uint8_t ov = (((uint8_t)a ^ (uint8_t)b) & ((uint8_t)a ^ rv)) & 0x80;
    if (ov) F |= OVF; else F &= ~OVF;
}

void CPU::writeIO(uint16_t addr, uint8_t v) {
    if (addr == 0xFF00) {
        // UART data -> stdout
        std::cout << (char)v << std::flush;
    } else {
        // default: store to memory (mirrored)
        mem[addr] = v;
    }
}

uint8_t CPU::readIO(uint16_t addr) {
    if (addr == 0xFF01) return 0x01; // UART ready
    return mem[addr];
}

std::string CPU::flags_str(uint8_t f) {
    std::ostringstream ss;
    ss << (f&CARRY?'C':'-')
       << (f&ZERO?'Z':'-')
       << (f&NEG?'N':'-')
       << (f&OVF?'V':'-');
    return ss.str();
}

void CPU::step() {
    uint16_t pc0 = PC;
    uint8_t op = fetch8();

    auto read = [&](uint16_t addr){ return (addr>=0xFF00? readIO(addr) : mem[addr]); };
    auto write = [&](uint16_t addr, uint8_t v) {
    if (addr >= 0xFF00) {
        writeIO(addr, v);
    } else {
        mem[addr] = v;
    }
};

    if (trace) {
        std::cout << std::hex << std::setw(4) << std::setfill('0') << pc0
                  << "  OP:" << std::setw(2) << (int)op
                  << "  A:" << std::setw(2) << (int)A
                  << "  B:" << std::setw(2) << (int)B
                  << "  F:" << flags_str(F) << "\n";
    }

    switch (op) {
        case 0x00: /*NOP*/ break;
        case 0x01: { A = fetch8(); setZN(A); } break; // LDA #imm
        case 0x02: { B = fetch8(); setZN(B); } break; // LDB #imm
        case 0x03: { uint16_t a = fetch16(); A = read(a); setZN(A);} break; // LDA abs
        case 0x04: { uint16_t a = fetch16(); B = read(a); setZN(B);} break; // LDB abs
        case 0x05: { uint16_t a = fetch16(); write(a, A);} break; // STA abs
        case 0x06: { uint16_t a = fetch16(); write(a, B);} break; // STB abs
        case 0x07: { uint16_t r = (uint16_t)A + (uint16_t)B; setAddFlags(A,B,r); A=(uint8_t)r; } break;
        case 0x08: { uint16_t r = (uint16_t)A - (uint16_t)B; setSubFlags(A,B,r); A=(uint8_t)r; } break;
        case 0x09: { uint16_t r = (uint16_t)A + 1; setAddFlags(A,1,r); A=(uint8_t)r; } break;
        case 0x0A: { uint16_t r = (uint16_t)A - 1; setSubFlags(A,1,r); A=(uint8_t)r; } break;
        case 0x0B: { uint16_t a = fetch16(); PC = a; } break; // JMP
        case 0x0C: { uint16_t a = fetch16(); if (F & ZERO) PC = a; } break; // JZ
        case 0x0D: { uint16_t a = fetch16(); if (!(F & ZERO)) PC = a; } break; // JNZ
        case 0x0E: { uint16_t r = (uint16_t)A - (uint16_t)B; setSubFlags(A,B,r);} break; // CMP A,B
        case 0x0F: { write(0xFF00, A); } break; // OUT A
        case 0x10: { uint8_t zp = fetch8(); A = read((uint16_t)zp); setZN(A);} break; // LDA zp
        case 0x11: { uint8_t zp = fetch8(); write((uint16_t)zp, A);} break; // STA zp
        case 0xFE: halted = true; break; // HLT
        case 0xFF: halted = true; break; // BRK alias
        default:
            std::cerr << "Unknown opcode " << std::hex << (int)op << " at " << pc0 << "\n";
            halted = true;
    }
}

void CPU::run(size_t max_steps) {
    size_t steps = 0;
    while (!halted && steps < max_steps) {
        step();
        steps++;
    }
}
