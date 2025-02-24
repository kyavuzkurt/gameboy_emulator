cmake_minimum_required(VERSION 3.10)
project(gameboy-emu)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

add_executable(gameboy-emu
    src/main.cpp
    src/cpu.cpp
    src/memory.cpp
    src/cartridge.cpp
    src/instructions.cpp
)

target_link_libraries(gameboy-emu PRIVATE SDL2::SDL2)
target_include_directories(gameboy-emu PRIVATE include)
