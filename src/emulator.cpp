#include "cpu.hpp"
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

static std::vector<uint8_t> read_bin(const std::string& path){
    std::ifstream f(path, std::ios::binary);
    if(!f) throw std::runtime_error("cannot open " + path);
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), {});
    return data;
}

static uint16_t parse_u16(const std::string& s){
    unsigned v=0;
    std::stringstream ss;
    ss << std::hex << s;
    if (s.rfind("0x",0)==0 || s.rfind("0X",0)==0) ss.clear(), ss.str(s.substr(2));
    if (s.rfind("0X",0)==0) {}
    ss >> std::hex >> v;
    if(!ss) { // maybe decimal
        ss.clear(); ss.str(s); ss >> v;
    }
    return (uint16_t)v;
}

int main(int argc, char** argv){
    if (argc<2){
        std::cerr << "Usage: emu <program.bin> [--pc ADDR] [--trace] [--steps N] [--dump ADDR LEN]\n";
        return 1;
    }

    std::string binpath = argv[1];
    uint16_t pc = 0x8000;
    bool trace = false;
    size_t steps = SIZE_MAX;
    int dump_after = -1; uint16_t dump_addr=0; int dump_len=0;

    for (int i=2;i<argc;i++){
        std::string a = argv[i];
        if (a=="--pc" && i+1<argc) pc = parse_u16(argv[++i]);
        else if (a=="--trace") trace = true;
        else if (a=="--steps" && i+1<argc) steps = std::stoul(argv[++i]);
        else if (a=="--dump" && i+2<argc) { dump_addr = parse_u16(argv[++i]); dump_len = std::stoi(argv[++i]); dump_after=1; }
    }

    CPU cpu; cpu.reset(pc); cpu.trace = trace;
    auto data = read_bin(binpath);
    cpu.load_binary(data, pc);
    cpu.run(steps);
    if (dump_after>0) cpu.dump(dump_addr, dump_len);
    return 0;
}
