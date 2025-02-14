#pragma once
#include <cstdint>


typedef struct{
   bool running;
   bool paused;
   uint64_t ticks;
} EmulatorState;

