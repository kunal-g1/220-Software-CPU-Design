// A tiny two-pass assembler for the cpu8 ISA.
// Usage: asm8 input.asm -o output.bin
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <cctype>
#include <algorithm>
#include <iomanip>

static std::string trim(const std::string& s){
    size_t a=0; while (a<s.size() && std::isspace((unsigned char)s[a])) a++;
    size_t b=s.size(); while (b>a && std::isspace((unsigned char)s[b-1])) b--;
    return s.substr(a,b-a);
}
static bool ishex(char c){ return std::isxdigit((unsigned char)c); }
static int tohex(char c){ if(c>='0'&&c<='9')return c-'0'; if(c>='a'&&c<='f')return 10+c-'a'; if(c>='A'&&c<='F')return 10+c-'A'; return -1; }

static int parse_num(const std::string& tok){
    // supports: decimal (42), hex (0x2A), hex with $ prefix ($2A)
    if (tok.size()>2 && tok[0]=='0' && (tok[1]=='x'||tok[1]=='X')) return std::stoi(tok,nullptr,16);
    if (tok.size()>1 && tok[0]=='$') return std::stoi(tok.substr(1),nullptr,16);
    return std::stoi(tok,nullptr,10);
}

struct Line { std::string text; int line_no; };
struct Unresolved { size_t pos; std::string label; int bytes; };

int main(int argc, char** argv){
    if (argc<2){ std::cerr<<"Usage: asm8 input.asm -o output.bin\n"; return 1; }
    std::string inpath=argv[1], outpath="a.bin";
    for(int i=2;i<argc;i++){ std::string a=argv[i]; if(a=="-o"&&i+1<argc) outpath=argv[++i]; }

    std::ifstream in(inpath);
    if(!in){ std::cerr<<"cannot open "<<inpath<<"\n"; return 1; }
    std::vector<Line> lines;
    std::string s; int ln=0;
    while(std::getline(in,s)){ ln++; lines.push_back({s,ln}); }

    std::unordered_map<std::string, uint16_t> labels;
    std::vector<uint8_t> out;
    std::vector<Unresolved> fixups;
    uint16_t org = 0x8000;
    uint16_t pc = org;

    auto lower = [](std::string t){ std::transform(t.begin(),t.end(),t.begin(),[](unsigned char c){return std::tolower(c);}); return t; };

    // Pass 1: labels & size
    for(auto& L: lines){
        std::string t=L.text;
        auto p = t.find(';'); if(p!=std::string::npos) t = t.substr(0,p);
        t = trim(t); if(t.empty()) continue;
        if (t.back()==':'){ labels[lower(trim(t.substr(0,t.size()-1)))] = pc; continue; }
        std::istringstream iss(t);
        std::string op; iss>>op; op=lower(op);
        if(op==".org"){ std::string n; iss>>n; pc = (uint16_t)parse_num(n); if(out.empty()) org=pc; continue; }
        if(op==".byte"){ std::string tok; while(std::getline(iss,tok,',')){ tok=trim(tok); pc+=1; } continue; }
        if(op==".word"){ std::string tok; while(std::getline(iss,tok,',')){ tok=trim(tok); pc+=2; } continue; }
        // instruction sizes
        if(op=="lda"||op=="ldb"){
            std::string arg; iss>>arg;
            if(!arg.size()) { std::cerr<<"syntax error line "<<L.line_no<<"\n"; return 1; }
            if(arg[0]=='#') pc+=2; // imm8
            else if(arg[0]=='$' || arg.find("0x")==0 || std::isdigit((unsigned char)arg[0]) ) pc+=3; // abs16 or zp8 (assume abs)
            else pc+=2; // zp label -> assume zp8, but treat as 2 for pass1 (we resolve to zp or abs later)
            continue;
        }
        if(op=="sta"||op=="stb"){ pc+= (3); continue; }
        if(op=="add"||op=="sub"||op=="inc"||op=="dec"||op=="cmp"||op=="out"||op=="hlt"||op=="nop"){ pc+=1; continue; }
        if(op=="jmp"||op=="jz"||op=="jnz"){ pc+=3; continue; }
        if(op=="lda_zp"||op=="sta_zp"){ pc+=2; continue; }
        std::cerr<<"Unknown op '"<<op<<"' at line "<<L.line_no<<"\n"; return 1;
    }

    // Pass 2: encode
    pc = org;
    for(auto& L: lines){
        std::string raw=L.text;
        auto p = raw.find(';'); if(p!=std::string::npos) raw = raw.substr(0,p);
        std::string t = trim(raw);
        if(t.empty()) continue;
        if (t.back()==':') continue;
        std::istringstream iss(t);
        std::string op; iss>>op; std::string opl=lower(op);
        auto emit8 = [&](uint8_t v){ out.push_back(v); pc++; };
        auto emit16 = [&](uint16_t v){ out.push_back((uint8_t)(v&0xff)); out.push_back((uint8_t)(v>>8)); pc+=2; };

        auto resolve = [&](const std::string& sym)->uint16_t{
            std::string k = lower(sym);
            if (labels.count(k)) return labels[k];
            // numeric?
            if (!sym.empty() && (std::isdigit((unsigned char)sym[0]) || sym[0]=='$' || (sym.size()>1 && sym[0]=='0' && (sym[1]=='x'||sym[1]=='X')))) {
                return (uint16_t)parse_num(sym);
            }
            std::cerr<<"Unresolved symbol '"<<sym<<"' at line "<<L.line_no<<"\n";
            exit(1);
        };

        if(opl==".org"){ std::string n; iss>>n; uint16_t v=(uint16_t)parse_num(n); while(pc<v){ emit8(0x00); } pc=v; continue; }
        if(opl==".byte"){ std::string tok; while(std::getline(iss,tok,',')){ tok=trim(tok); emit8((uint8_t)parse_num(tok)); } continue; }
        if(opl==".word"){ std::string tok; while(std::getline(iss,tok,',')){ tok=trim(tok); emit16((uint16_t)parse_num(tok)); } continue; }

        if(opl=="lda"){
            std::string arg; iss>>arg;
            if(arg[0]=='#'){ emit8(0x01); emit8((uint8_t)parse_num(arg.substr(1))); }
            else if(arg.size()<=3 && arg[0]!='$' && !(arg.size()>1 && arg[0]=='0' && (arg[1]=='x'||arg[1]=='X')) && !std::isdigit((unsigned char)arg[0])){
                // label -> decide zp vs abs at link-time (assume abs)
                uint16_t addr = resolve(arg);
                if (addr < 0x0100) { emit8(0x10); emit8((uint8_t)addr); }
                else { emit8(0x03); emit16(addr); }
            } else {
                uint16_t addr = resolve(arg);
                if (addr < 0x0100) { emit8(0x10); emit8((uint8_t)addr); }
                else { emit8(0x03); emit16(addr); }
            }
            continue;
        }
        if(opl=="ldb"){
            std::string arg; iss>>arg;
            if(arg[0]=='#'){ emit8(0x02); emit8((uint8_t)parse_num(arg.substr(1))); }
            else { uint16_t addr = resolve(arg); if(addr<0x0100){ emit8(0x04); emit16(addr); } else { emit8(0x04); emit16(addr);} }
            continue;
        }
        if(opl=="sta"){ std::string arg; iss>>arg; uint16_t addr = resolve(arg); if(addr<0x0100){ emit8(0x11); emit8((uint8_t)addr);} else { emit8(0x05); emit16(addr);} continue; }
        if(opl=="stb"){ std::string arg; iss>>arg; uint16_t addr = resolve(arg); if(addr<0x0100){ emit8(0x06); emit16(addr);} else { emit8(0x06); emit16(addr);} continue; }
        if(opl=="add"){ emit8(0x07); continue; }
        if(opl=="sub"){ emit8(0x08); continue; }
        if(opl=="inc"){ emit8(0x09); continue; }
        if(opl=="dec"){ emit8(0x0A); continue; }
        if(opl=="cmp"){ emit8(0x0E); continue; }
        if(opl=="out"){ emit8(0x0F); continue; }
        if(opl=="nop"){ emit8(0x00); continue; }
        if(opl=="hlt"){ emit8(0xFE); continue; }
        if(opl=="jmp"||opl=="jz"||opl=="jnz"){
            std::string arg; iss>>arg; uint16_t addr = resolve(arg);
            if(opl=="jmp") emit8(0x0B);
            else if(opl=="jz") emit8(0x0C);
            else emit8(0x0D);
            emit16(addr);
            continue;
        }
        std::cerr<<"Unknown op "<<op<<" at line "<<L.line_no<<"\n"; return 1;
    }

    std::ofstream outb(outpath, std::ios::binary);
    outb.write((char*)out.data(), out.size());
    std::cerr<<"Assembled "<<out.size()<<" bytes to "<<outpath<<"\n";
    return 0;
}
