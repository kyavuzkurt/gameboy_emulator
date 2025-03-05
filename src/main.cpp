#include "cartridge.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "emu.hpp"
#include <SDL2/SDL.h>
#include <iostream>

static EmulatorState ctx;
static Cartridge* cart = nullptr;
static MemoryBus* memory = nullptr;
static CPU* cpu = nullptr;

bool init_system(const char* rom_path) {
    try {
        // Create and load cartridge
        cart = new Cartridge(rom_path);
        
        // Initialize memory and CPU
        memory = new MemoryBus(*cart);
        cpu = new CPU(*memory);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

void cleanup_system() {
    delete cpu;
    delete memory;
    delete cart;
}

bool cpu_step() {
    try {
        cpu->tick();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CPU error: " << e.what() << std::endl;
        return false;
    }
}

// New function to handle cycle-accurate timing
bool system_tick() {
    // Execute one CPU cycle
    if (!cpu_step()) {
        return false;
    }
    
    // Update other components based on CPU cycles
    // Each component will check the CPU's current cycle count
    // and update its state accordingly
    
    // Example: Update GPU every cycle
    // gpu->update(cpu->getCycles());
    
    // Example: Update timer
    // timer->update(cpu->getCycles());
    
    return true;
}

int emu_run(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <rom_file>" << std::endl;
        return -1;
    }

    if (!init_system(argv[1])) {
        std::cerr << "Failed to initialize system" << std::endl;
        return -2;
    }

    std::cout << "System initialized..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return -3;
    }
    std::cout << "SDL initialized" << std::endl;

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;

    const uint64_t GB_CLOCK_SPEED = 4194304; // GameBoy CPU runs at ~4.19 MHz
    const uint64_t CYCLES_PER_FRAME = GB_CLOCK_SPEED / 60; // ~69905 cycles per frame at 60 FPS
    
    uint64_t frame_cycles = 0;
    uint64_t last_time = SDL_GetTicks();
    uint64_t frame_time = 0;
    
    SDL_Event event;
    while (ctx.running) {
        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    ctx.running = false;
                    break;
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym) {
                        case SDLK_ESCAPE:
                            ctx.running = false;
                            break;
                        case SDLK_SPACE:
                            ctx.paused = !ctx.paused;
                            break;
                    }
                    break;
            }
        }

        if (ctx.paused) {
            SDL_Delay(10);
            continue;
        }

        // Get current time
        uint64_t current_time = SDL_GetTicks();
        frame_time += current_time - last_time;
        last_time = current_time;
        
        // Run CPU cycles for one frame
        while (frame_cycles < CYCLES_PER_FRAME && ctx.running && !ctx.paused) {
            if (!system_tick()) {
                std::cerr << "CPU Stopped" << std::endl;
                ctx.running = false;
                break;
            }
            
            frame_cycles++;
        }
        
        // Reset frame cycle counter
        if (frame_cycles >= CYCLES_PER_FRAME) {
            frame_cycles = 0;
            
            // Render screen here
            // ...
            
            // Cap to 60 FPS
            if (frame_time < 16) {
                SDL_Delay(16 - frame_time);
            }
            frame_time = 0;
        }
        
        ctx.ticks++;
    }

    // Cleanup
    SDL_Quit();
    cleanup_system();

    return 0;
}

int main(int argc, char** argv) {
    return emu_run(argc, argv);
} 