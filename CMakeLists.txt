cmake_minimum_required(VERSION 3.10)
project(cpu8 CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(emu src/emulator.cpp src/cpu.cpp)
add_executable(asm8 src/assembler.cpp)
