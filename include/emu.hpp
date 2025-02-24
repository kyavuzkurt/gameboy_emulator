#pragma once
#include <cstdint>


struct EmulatorState {
    bool running;
    bool paused;
    uint64_t ticks;
    // Add any other emulator state you need
};

