#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <vector>

struct CPU {
    // Registers
    uint8_t  A{0}, B{0};
    uint16_t PC{0x8000};
    uint16_t SP{0x7FFF}; // reserved
    // Flags: bit0=C, bit1=Z, bit2=N, bit3=V
    uint8_t  F{0};

    // Memory 64KB
    std::array<uint8_t, 65536> mem{};

    bool halted{false};
    bool trace{false};

    // Helpers
    enum Flag { CARRY=1, ZERO=2, NEG=4, OVF=8 };
    void reset(uint16_t pc=0x8000);
    void load_binary(const std::vector<uint8_t>& bin, uint16_t addr=0x8000);
    void dump(uint16_t addr, size_t len);
    uint8_t  fetch8();
    uint16_t fetch16();
    void setZN(uint8_t v);
    void setAddFlags(uint16_t a, uint16_t b, uint16_t r);
    void setSubFlags(uint16_t a, uint16_t b, uint16_t r);
    void writeIO(uint16_t addr, uint8_t v);
    uint8_t  readIO(uint16_t addr);
    void step();
    void run(size_t max_steps = SIZE_MAX);
    static std::string flags_str(uint8_t F);
};
