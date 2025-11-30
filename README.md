# CPU8 — Software CPU Project (C++)

This repository implements a simple 8-bit software CPU for our Computer Architecture assignment.

## Team Members
- Kunal Rajesh Goel
- Adithya Thirumalai
- Sunny Dilipkumar Shah
- Aruna Samhitha

## Structure
- `src/` — C++ source code
- `programs/` — Assembly examples

This repo contains:
- CPU schematic (ASCII), ISA spec, instruction encoding, addressing modes, flags, memory map
- Emulator (registers, ALU, control unit, memory, memory‑mapped I/O, load/run/memdump, step/trace)
- Assembler (two‑pass with labels & numeric literals; emits a flat binary)
- Programs: **hello**, **fibonacci**, **timer/trace** (shows Fetch/Compute/Store via `--trace`)

## Build

```bash
mkdir -p build && cd build
cmake .. && make -j
```

This builds:
- `emu`  — the CPU emulator
- `asm8` — the assembler

## Run

Assemble a program and run it:

```bash
# from repo root
./build/asm8 programs/hello.asm -o hello.bin
./build/emu hello.bin --trace        # run with instruction trace
./build/emu hello.bin --dump 0x7ff0 32   # dump 32 bytes around 0x7ff0
```

## Architecture Overview (Schematic)

```
              +---------------------------+
              |         CONTROL           |
              |  fetch -> decode -> exec  |
              +-------------+-------------+
                            |
         +------------------v------------------+
         |                ALU                  |
         |  A,B -> {ADD,SUB,AND,OR,XOR,INC,DEC}|
         |      Flags: C Z N V                 |
         +-----------+----------+--------------+
                     |          |
                   +--+       +--+
                   |A |       |B |   8-bit registers
                   +--+       +--+
                     \          /
                      \        /
                       \      /
                        v    v
                      +--------+
                      |  BUS   |
                      +--------+
                          |
      +-------------------+--------------------+
      |                                        |
+-----v------+                           +-----v------+
|   MEMORY   |                           |  I/O MAP   |
| 64KB RAM   |                           | 0xFF00 UART|
+------------+                           +------------+

PC (16-bit), SP (16-bit), FLAGS {C,Z,N,V}
```

## ISA

- **Data width:** 8-bit
- **Address width:** 16-bit
- **Registers:** A, B (8-bit), PC (16-bit), SP (16-bit), FLAGS {C,Z,N,V}
- **Instruction format:** 1-byte opcode + zero/one/two operand bytes (little endian for 16-bit)
- **Addressing modes:** immediate `#imm8`, absolute `$abs16`, zero-page `$zp8`

### Opcodes

| Opcode | Mnemonic                  | Operands           | Description             |
|-------:|---------------------------|--------------------|-------------------------|
| 0x00   | NOP                       | —                  | No op                   |
| 0x01   | LDA  #imm8                | imm8               | A ← imm8                |
| 0x02   | LDB  #imm8                | imm8               | B ← imm8                |
| 0x03   | LDA  $abs                 | lo hi              | A ← [abs]               |
| 0x04   | LDB  $abs                 | lo hi              | B ← [abs]               |
| 0x05   | STA  $abs                 | lo hi              | [abs] ← A               |
| 0x06   | STB  $abs                 | lo hi              | [abs] ← B               |
| 0x07   | ADD  A,B                  | —                  | A ← A+B                 |
| 0x08   | SUB  A,B                  | —                  | A ← A−B                 |
| 0x09   | INC  A                    | —                  | A ← A+1                 |
| 0x0A   | DEC  A                    | —                  | A ← A−1                 |
| 0x0B   | JMP  $abs                 | lo hi              | PC ← abs                |
| 0x0C   | JZ   $abs                 | lo hi              | jump if Z=1             |
| 0x0D   | JNZ  $abs                 | lo hi              | jump if Z=0             |
| 0x0E   | CMP  A,B                  | —                  | set flags on (A−B)      |
| 0x0F   | OUT  A                    | —                  | write A → 0xFF00 (UART) |
| 0x10   | LDA  $zp                  | zp                 | A ← [0x0000+zp]         |
| 0x11   | STA  $zp                  | zp                 | [0x0000+zp] ← A         |
| 0xFE   | HLT                       | —                  | halt                    |

**Flags:**  
- **Z** (zero) if result==0  
- **N** (negative) = bit7 of result  
- **C** (carry/borrow) on add/sub  
- **V** (overflow) on add/sub (signed)

### Memory Map

```
0000–00FF : Zero page (fast scratch)
0100–7FFF : General RAM
8000–FEFF : Program load area (default PC starts at 0x8000)
FF00      : UART_DATA (write char)
FF01      : UART_STATUS (bit0=1 always ready)
FF10–FF1F : Reserved for future MMIO
FFFE–FFFF : Reserved
```

## Assembler Syntax

- Comments start with `;` or `#`
- Labels: `start:`
- Numeric literals: decimal (`42`), hex (`0x2A` or `$2A`)
- Directives: `.org <addr>`, `.byte <b1, b2, ...>`, `.word <w1, w2, ...>`
- Addressing: immediate `#n`, absolute `$addr`, zero-page `$zz`

Example:
```asm
.org $8000
start:
    LDA #72
    OUT
    HLT
```

## Demo Programs

- `programs/hello.asm` — prints “Hello, World!\n”
- `programs/fib.asm` — prints first 10 Fibonacci numbers as **hex bytes**, separated by spaces, newline at end
- `programs/timer.asm` — tiny loop that demonstrates Fetch/Compute/Store when run with `--trace`

## Emulator Usage

```
emu <program.bin> [--pc 0x8000] [--trace] [--steps N] [--dump <addr> <len>]
```

- `--trace` shows per‑instruction fetch/decode/execute with PC, opcode, A, B, FLAGS.
- `--steps N` limits steps (useful for debugging infinite loops).
- Memory dumps are in hex. IO writes to 0xFF00 print to stdout.