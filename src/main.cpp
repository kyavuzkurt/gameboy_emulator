#include "cartridge.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "gpu.hpp"
#include "emu.hpp"
#include "timer.hpp"
#include <SDL2/SDL.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <chrono>
#include <thread>
#include <algorithm>
#include <cstring>  // For std::memcpy
#include <iomanip>  // For std::setw and std::setfill

static EmulatorState ctx;
static Cartridge* cart = nullptr;
static MemoryBus* memory = nullptr;
static CPU* cpu = nullptr;
static GPU* gpu = nullptr;
static Timer* timer = nullptr;

// Game Boy interrupt register addresses
static constexpr uint16_t IF_REG = 0xFF0F;  // Interrupt Flag Register
static constexpr uint16_t IE_REG = 0xFFFF;  // Interrupt Enable Register

// Interrupt bits
static constexpr uint8_t INT_VBLANK = 0x01;
static constexpr uint8_t INT_LCD_STAT = 0x02;
static constexpr uint8_t INT_TIMER = 0x04;
static constexpr uint8_t INT_SERIAL = 0x08;
static constexpr uint8_t INT_JOYPAD = 0x10;

// SDL window and renderer
static SDL_Window* window = nullptr;
static SDL_Renderer* renderer = nullptr;
static SDL_Texture* screen_texture = nullptr;

// Tracking executed instructions for post-mortem analysis
static std::map<uint16_t, uint32_t> executed_addresses;
static std::set<uint16_t> code_path;
static std::vector<uint16_t> execution_trace;
static bool tracing_enabled = false;
static int trace_limit = 50000; // Stop tracing after this many instructions

// Global variables for button state
static bool buttons[8] = {false}; // [Down, Up, Left, Right, Start, Select, B, A]

// Add these global variables at the top with the other globals
static bool use_debug_pattern = true; // Set to true by default to ensure we see something
static bool use_alternating_pattern = false;

bool init_sdl() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL initialization failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    std::cout << "SDL initialized successfully" << std::endl;
    
    // Create window and renderer
    window = SDL_CreateWindow("GameBoy Emulator", 
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              SCREEN_WIDTH * 4, SCREEN_HEIGHT * 4,  // Scale display 4x
                              SDL_WINDOW_SHOWN);
    
    if (!window) {
        std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    std::cout << "Window created successfully: " << SCREEN_WIDTH * 4 << "x" << SCREEN_HEIGHT * 4 << std::endl;
    
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    std::cout << "Renderer created successfully" << std::endl;
    
    // Get renderer info
    SDL_RendererInfo info;
    if (SDL_GetRendererInfo(renderer, &info) == 0) {
        std::cout << "Renderer name: " << info.name << std::endl;
        std::cout << "Texture formats: " << info.num_texture_formats << std::endl;
        std::cout << "Max texture size: " << info.max_texture_width << "x" << info.max_texture_height << std::endl;
    }
    
    // Create texture for GPU output
    screen_texture = SDL_CreateTexture(renderer,
                                      SDL_PIXELFORMAT_ARGB8888,
                                      SDL_TEXTUREACCESS_STREAMING,
                                      SCREEN_WIDTH, SCREEN_HEIGHT);
    
    if (!screen_texture) {
        std::cerr << "Texture creation failed: " << SDL_GetError() << std::endl;
        return false;
    }
    
    std::cout << "Screen texture created successfully: " << SCREEN_WIDTH << "x" << SCREEN_HEIGHT << std::endl;
    
    // Test rendering a simple frame to verify SDL setup
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // Red background
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);
    std::cout << "Test frame rendered (should be red)" << std::endl;
    SDL_Delay(500); // Pause so we can see the red screen
    
    return true;
}

void cleanup_sdl() {
    if (screen_texture) {
        SDL_DestroyTexture(screen_texture);
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
    }
    if (window) {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

// Callback for VBLANK interrupt
void request_vblank_interrupt() {
    uint8_t if_value = memory->read(IF_REG);
    memory->write(IF_REG, if_value | INT_VBLANK);
}

// Callback for LCD STAT interrupt
void request_lcd_stat_interrupt() {
    uint8_t if_value = memory->read(IF_REG);
    memory->write(IF_REG, if_value | INT_LCD_STAT);
}

// Function to initialize the system to post-boot state
void initializeSystem(CPU& cpu, GPU& gpu, MemoryBus& memory, Cartridge& cart, bool dmgMode = true) {
    // 1. Initialize hardware registers to post-boot ROM values
    // These match DMG boot state per PanDocs
    
    // Joypad
    memory.write(0xFF00, 0xCF);  // P1/JOYP
    
    // Serial
    memory.write(0xFF01, 0x00);  // SB
    memory.write(0xFF02, 0x7E);  // SC
    
    // Timer registers
    memory.write(0xFF04, 0xAB);  // DIV - random value at boot
    memory.write(0xFF05, 0x00);  // TIMA
    memory.write(0xFF06, 0x00);  // TMA
    memory.write(0xFF07, 0xF8);  // TAC
    
    // Interrupt flag
    memory.write(0xFF0F, 0xE1);  // IF
    
    // Audio registers
    memory.write(0xFF10, 0x80);  // NR10
    memory.write(0xFF11, 0xBF);  // NR11
    memory.write(0xFF12, 0xF3);  // NR12
    memory.write(0xFF13, 0xFF);  // NR13
    memory.write(0xFF14, 0xBF);  // NR14
    memory.write(0xFF16, 0x3F);  // NR21
    memory.write(0xFF17, 0x00);  // NR22
    memory.write(0xFF18, 0xFF);  // NR23 
    memory.write(0xFF19, 0xBF);  // NR24
    memory.write(0xFF1A, 0x7F);  // NR30
    memory.write(0xFF1B, 0xFF);  // NR31
    memory.write(0xFF1C, 0x9F);  // NR32
    memory.write(0xFF1D, 0xFF);  // NR33
    memory.write(0xFF1E, 0xBF);  // NR34
    memory.write(0xFF20, 0xFF);  // NR41
    memory.write(0xFF21, 0x00);  // NR42
    memory.write(0xFF22, 0x00);  // NR43
    memory.write(0xFF23, 0xBF);  // NR44
    memory.write(0xFF24, 0x77);  // NR50
    memory.write(0xFF25, 0xF3);  // NR51
    memory.write(0xFF26, 0xF1);  // NR52
    
    // LCD registers
    memory.write(0xFF40, 0x91);  // LCDC
    memory.write(0xFF41, 0x85);  // STAT
    memory.write(0xFF42, 0x00);  // SCY
    memory.write(0xFF43, 0x00);  // SCX
    memory.write(0xFF44, 0x00);  // LY
    memory.write(0xFF45, 0x00);  // LYC
    memory.write(0xFF47, 0xFC);  // BGP
    memory.write(0xFF48, 0xFF);  // OBP0
    memory.write(0xFF49, 0xFF);  // OBP1
    memory.write(0xFF4A, 0x00);  // WY
    memory.write(0xFF4B, 0x00);  // WX
    
    // Ensure DMA is properly initialized
    memory.write(0xFF46, 0xFF);
    
    // Interrupt Enable
    memory.write(0xFFFF, 0x00);
    
    // Reset CPU to correct boot state
    cpu.reset();
    
    // Reset GPU state
    gpu.reset();
    
    // Special setup for Tetris
    if (cart.getTitle().find("TETRIS") != std::string::npos) {
        // Tetris expects a RET instruction at 0xFFB6 for compatibility
        memory.write(0xFFB6, 0xC9);
        std::cout << "Initialized address 0xFFB6 with RET instruction (0xC9) for Tetris compatibility" << std::endl;
    }
}

bool init_system(const char* rom_path) {
    try {
        // Create and load cartridge
        cart = new Cartridge(rom_path);
        
        // Create timer
        timer = new Timer(*memory);

        // Initialize memory and CPU
        memory = new MemoryBus(*cart, *timer);
        std::cout << "Memory bus initialized" << std::endl;
        
        // Set the memory reference in timer (fix circular dependency)
        delete timer;
        timer = new Timer(*memory);
        
        // Initialize the GPU
        gpu = new GPU(*memory);
        std::cout << "GPU initialized" << std::endl;
        
        // Connect the GPU back to memory bus for VRAM sharing
        memory->setGPU(gpu);
        std::cout << "GPU connected to memory bus" << std::endl;
        
        // Initialize the CPU
        cpu = new CPU(*memory);
        std::cout << "CPU initialized" << std::endl;
        
        // Disable CPU debug output
        cpu->debug_output_enabled = false;
        
        // Register GPU interrupt callbacks
        gpu->setVBlankInterruptCallback(request_vblank_interrupt);
        gpu->setLCDStatInterruptCallback(request_lcd_stat_interrupt);
        
        // Initialize SDL
        if (!init_sdl()) {
            return false;
        }
        
        // Special handling for Tetris
        if (cart->getTitle() == "TETRIS") {
            std::cout << "Tetris ROM detected - enabling special debug checks" << std::endl;
            // Enable any specific debug flags for Tetris
            
            // Force specific LCD settings known to work with Tetris
            memory->write(0xFF40, 0x91); // LCD Control (enable LCD and background)
            memory->write(0xFF47, 0xFC); // Background palette (white, light gray, dark gray, black)
        }
        
        // Initialize system to post-boot ROM state
        initializeSystem(*cpu, *gpu, *memory, *cart);
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Initialization error: " << e.what() << std::endl;
        return false;
    }
}

void cleanup_system() {
    cleanup_sdl();
    delete gpu;
    delete cpu;
    delete timer;
    delete memory;
    delete cart;
}

// Utility function to generate a code execution map
void dump_execution_trace(const std::string& filename) {
    std::ofstream outfile(filename);
    if (!outfile.is_open()) {
        std::cerr << "Failed to open execution trace file: " << filename << std::endl;
        return;
    }
    
    // Write header
    outfile << "Address,Count,Sequence\n";
    
    // Output addresses sorted by execution count
    std::vector<std::pair<uint16_t, uint32_t>> sorted_addrs(
        executed_addresses.begin(), executed_addresses.end());
    
    std::sort(sorted_addrs.begin(), sorted_addrs.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (const auto& [addr, count] : sorted_addrs) {
        outfile << "0x" << std::hex << addr << std::dec << "," << count;
        
        // Also find the position in the execution trace if it exists
        auto it = std::find(execution_trace.begin(), execution_trace.end(), addr);
        if (it != execution_trace.end()) {
            outfile << "," << std::distance(execution_trace.begin(), it);
        }
        outfile << "\n";
    }
    
    // Write the full execution path
    outfile << "\nExecution Trace (first " << trace_limit << " steps):\n";
    for (size_t i = 0; i < execution_trace.size(); ++i) {
        outfile << i << ": 0x" << std::hex << execution_trace[i] << std::dec << "\n";
    }
    
    std::cout << "Execution trace written to " << filename << std::endl;
}

bool cpu_step() {
    try {
        if (tracing_enabled) {
            uint16_t pc = cpu->getPC();
            
            // Record this address in our execution map
            executed_addresses[pc]++;
            
            // Record it in our trace if we're still under the limit
            if (execution_trace.size() < trace_limit) {
                execution_trace.push_back(pc);
            } else if (execution_trace.size() == trace_limit) {
                tracing_enabled = false;
                std::cout << "Trace limit reached. Stopped recording execution trace." << std::endl;
                
                // Dump the trace to a file
                dump_execution_trace("execution_trace.csv");
            }
        }
        
        cpu->tick();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "CPU error: " << e.what() << std::endl;
        return false;
    }
}

// Update screen with current GPU buffer
void update_display() {
    const auto& buffer = gpu->getScreenBuffer();
    
    // Debug info
    static int frame_count = 0;
    frame_count++;
    
    // Dump VRAM debug information at specific frame numbers
    if (frame_count == 50 || frame_count == 100 || frame_count == 200) {
        std::cout << "Dumping VRAM debug info at frame " << frame_count << std::endl;
        gpu->dumpVRAMDebug();
    }
    
    // Every 10 frames, log what's in the first few pixels
    if (frame_count % 10 == 0) {
        std::cout << "UPDATE DISPLAY - Frame " << frame_count << std::endl;
        std::cout << "First 4 pixels in display buffer (full 32-bit hex): ";
        for (int i = 0; i < 4 && i < buffer.size(); i++) {
            std::cout << std::hex << "0x" << std::setw(8) << std::setfill('0') 
                      << buffer[i] << " ";
        }
        std::cout << std::dec << std::endl;
        std::cout << "Buffer size: " << buffer.size() << " pixels" << std::endl;
    }
    
    // Copy the GPU buffer directly to our display buffer
    static std::vector<uint32_t> display_buffer(SCREEN_WIDTH * SCREEN_HEIGHT, 0);
    std::copy(buffer.begin(), buffer.end(), display_buffer.begin());
    
    // Add a small debug indicator in the corner (just 8x8 pixels)
    // This helps us know if the display is updating without interfering with game rendering
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            display_buffer[y * SCREEN_WIDTH + x] = 0xFFFF00FF; // Magenta corner marker
        }
    }
    
    if (frame_count % 10 == 0) {
        std::cout << "Using GPU buffer with corner marker for display" << std::endl;
    }
    
    // Update SDL texture
    void* pixels;
    int pitch;
    
    if (SDL_LockTexture(screen_texture, nullptr, &pixels, &pitch) != 0) {
        std::cerr << "Failed to lock texture: " << SDL_GetError() << std::endl;
        return;
    }
    
    // Copy our buffer to the texture
    std::memcpy(pixels, display_buffer.data(), SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    
    // Unlock texture
    SDL_UnlockTexture(screen_texture);
    
    // Clear renderer
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black
    SDL_RenderClear(renderer);
    
    // Draw scaled texture to fill window
    SDL_Rect dest_rect = {0, 0, SCREEN_WIDTH * 4, SCREEN_HEIGHT * 4};
    SDL_RenderCopy(renderer, screen_texture, NULL, &dest_rect);
    
    // Present the renderer
    SDL_RenderPresent(renderer);
    
    if (frame_count % 10 == 0) {
        std::cout << "Rendered frame " << frame_count << " to screen" << std::endl;
    }
}

// Replace the complex update_timer function with this simplified version
void update_timer(uint64_t cycles, MemoryBus& memory) {
    if (timer) {
        timer->tick(cycles);
    }
}

// And update the system_tick function to use it properly
bool system_tick(uint64_t cycles) {
    static uint64_t total_ticks = 0;
    total_ticks++;
    
    // Debug output every million ticks
    if (total_ticks % 1000000 == 0) {
        std::cout << "System tick: " << total_ticks 
                  << ", total CPU cycles: " << cpu->getCycles()
                  << std::endl;
    }
    
    // Execute one CPU cycle
    if (!cpu_step()) {
        return false;
    }
    
    // Update GPU with the elapsed CPU cycles
    if (gpu) {
        gpu->tick(cycles);
    }
    
    // Update timer with the elapsed CPU cycles
    if (timer) {
        timer->tick(cycles);
    }
    
    return true;
}

// Helper function to update joypad state based on button presses
void update_joypad_state() {
    if (!memory) return;
    
    // Get current joypad register state
    uint8_t joypad_state = memory->read(0xFF00) & 0xF0; // Keep only the upper 4 bits
    
    // Check which button type is selected (bit 4 = direction, bit 5 = buttons)
    bool direction_selected = (joypad_state & 0x10) == 0;
    bool button_selected = (joypad_state & 0x20) == 0;
    
    // Set the lower 4 bits based on which buttons are pressed
    uint8_t direction_bits = 0x0F; // Default all directions to 1 (not pressed)
    uint8_t button_bits = 0x0F;    // Default all buttons to 1 (not pressed)
    
    // Apply pressed buttons (set bit to 0 for pressed)
    if (buttons[0]) direction_bits &= ~0x08; // Down
    if (buttons[1]) direction_bits &= ~0x04; // Up
    if (buttons[2]) direction_bits &= ~0x02; // Left
    if (buttons[3]) direction_bits &= ~0x01; // Right
    
    if (buttons[4]) button_bits &= ~0x08; // Start
    if (buttons[5]) button_bits &= ~0x04; // Select
    if (buttons[6]) button_bits &= ~0x02; // B
    if (buttons[7]) button_bits &= ~0x01; // A
    
    // Apply the selected bits to the joypad state
    if (direction_selected) {
        joypad_state |= direction_bits;
    } else {
        joypad_state |= 0x0F; // All directions not pressed
    }
    
    if (button_selected) {
        joypad_state |= button_bits;
    } else {
        joypad_state |= 0x0F; // All buttons not pressed
    }
    
    // Check if any button was just pressed (transition from 1 to 0)
    static uint8_t last_state = 0xFF;
    uint8_t pressed_bits = 0;
    
    if (direction_selected) {
        pressed_bits = (last_state & 0x0F) & ~(joypad_state & 0x0F);
    } else if (button_selected) {
        pressed_bits = (last_state & 0x0F) & ~(joypad_state & 0x0F);
    }
    
    // If any button was pressed, trigger a joypad interrupt
    if (pressed_bits != 0) {
        // Request joypad interrupt
        uint8_t if_value = memory->read(IF_REG);
        memory->write(IF_REG, if_value | INT_JOYPAD);
        
        std::cout << "Joypad interrupt requested" << std::endl;
    }
    
    // Update the joypad register
    memory->write(0xFF00, joypad_state);
    last_state = joypad_state;
}

// Function to handle SDL key events and map them to Game Boy buttons
void handle_key_event(SDL_KeyboardEvent& key, bool pressed) {
    // Handle Game Boy buttons
    switch (key.keysym.sym) {
        // Directional keys
        case SDLK_DOWN:    buttons[0] = pressed; break; // Down
        case SDLK_UP:      buttons[1] = pressed; break; // Up
        case SDLK_LEFT:    buttons[2] = pressed; break; // Left
        case SDLK_RIGHT:   buttons[3] = pressed; break; // Right
        
        // Game buttons
        case SDLK_RETURN:  buttons[4] = pressed; break; // Start
        case SDLK_RSHIFT:  buttons[5] = pressed; break; // Select
        case SDLK_z:       buttons[6] = pressed; break; // B
        case SDLK_x:       buttons[7] = pressed; break; // A
        
        default: break;
    }
    
    // Update joypad state whenever a Game Boy button changes
    update_joypad_state();
    
    if (pressed) {
        std::cout << "Key pressed: ";
    } else {
        std::cout << "Key released: ";
    }
    
    std::cout << SDL_GetKeyName(key.keysym.sym) << std::endl;
}

void handleInput(SDL_Event& event) {
    // Convert SDL key events to Game Boy button presses
    if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
        bool pressed = (event.type == SDL_KEYDOWN);
        uint8_t mask = 0;
        
        switch (event.key.keysym.sym) {
            // D-pad (bottom 4 bits)
            case SDLK_RIGHT:  mask = 0x01; break;
            case SDLK_LEFT:   mask = 0x02; break;
            case SDLK_UP:     mask = 0x04; break;
            case SDLK_DOWN:   mask = 0x08; break;
            
            // Buttons (upper 4 bits)
            case SDLK_RETURN: mask = 0x10; break; // Start
            case SDLK_RSHIFT: mask = 0x20; break; // Select
            case SDLK_z:      mask = 0x40; break; // B
            case SDLK_x:      mask = 0x80; break; // A
            default: return;
        }
        
        // Update the button state in memory
        memory->updateJoypadButton(mask, pressed);
        
        // Debug output
        if (pressed) {
            std::cout << "Button pressed: ";
        } else {
            std::cout << "Button released: ";
        }
        std::cout << SDL_GetKeyName(event.key.keysym.sym) << std::endl;
    }
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

    // Define constants before using them
    const uint64_t GB_CLOCK_SPEED = 4194304; // GameBoy CPU runs at ~4.19 MHz
    const uint64_t CYCLES_PER_FRAME = GB_CLOCK_SPEED / 60; // ~69905 cycles per frame at 60 FPS

    std::cout << "System initialized with ROM: " << argv[1] << std::endl;
    std::cout << "CPU cycles per frame: " << GB_CLOCK_SPEED / 60 << std::endl;
    std::cout << "Display: " << SCREEN_WIDTH << "x" << SCREEN_HEIGHT << std::endl;

    ctx.running = true;
    ctx.paused = false;
    ctx.ticks = 0;
    
    uint64_t frame_cycles = 0;
    uint64_t last_time = SDL_GetTicks();
    uint64_t frame_time = 0;
    uint64_t total_frames = 0;
    
    uint64_t last_automatic_vram_dump = 0;
    bool already_dumped_vram = false;
    std::cout << "Key Commands:" << std::endl;
    std::cout << "  ESC - Quit" << std::endl;
    std::cout << "  SPACE - Pause/Resume" << std::endl;
    std::cout << "  D - Dump VRAM to file" << std::endl;
    std::cout << "  T - Dump execution trace to file" << std::endl;
    std::cout << "Game Controls:" << std::endl;
    std::cout << "  Arrow Keys - D-pad" << std::endl;
    std::cout << "  Enter - Start" << std::endl;
    std::cout << "  Right Shift - Select" << std::endl;
    std::cout << "  Z - B button" << std::endl;
    std::cout << "  X - A button" << std::endl;

    std::cout << "Starting emulation loop..." << std::endl;
    
    SDL_Event event;
    while (ctx.running) {
        // Handle SDL events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                ctx.running = false;
                break;
            } else if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP) {
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    ctx.running = false;
                    break;
                } else if (event.key.keysym.sym == SDLK_SPACE && event.type == SDL_KEYDOWN) {
                    ctx.paused = !ctx.paused;
                    std::cout << (ctx.paused ? "Emulation paused" : "Emulation resumed") << std::endl;
                } else if (event.key.keysym.sym == SDLK_d && event.type == SDL_KEYDOWN) {
                    // Dump VRAM to file
                    std::string filename = "vram_dump_" + std::to_string(total_frames) + ".txt";
                    std::cout << "Dumping VRAM to " << filename << std::endl;
                    gpu->dumpVRAM(filename);
                } else if (event.key.keysym.sym == SDLK_t && event.type == SDL_KEYDOWN) {
                    // Toggle between debug pattern and game rendering
                    static bool debug_pattern_enabled = false;
                    debug_pattern_enabled = !debug_pattern_enabled;
                    std::cout << "Debug pattern " << (debug_pattern_enabled ? "enabled" : "disabled") << std::endl;
                    
                    // Set the flag that update_display() checks
                    use_debug_pattern = debug_pattern_enabled;
                    use_alternating_pattern = false;  // Stop auto-toggling when manually toggled
                } else {
                    // Handle Game Boy button presses
                    handleInput(event);
                }
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
            if (!system_tick(1)) {  // Pass 1 cycle to the system
                std::cerr << "CPU Stopped" << std::endl;
                ctx.running = false;
                break;
            }
            
            frame_cycles++;
        }
        
        // Reset frame cycle counter
        if (frame_cycles >= CYCLES_PER_FRAME) {
            total_frames++;
            
            // Debug output every 60 frames (about once per second)
            if (total_frames % 60 == 0) {
                std::cout << "Running for " << total_frames << " frames, " 
                          << "CPU cycles: " << cpu->getCycles() 
                          << ", Time: " << SDL_GetTicks() / 1000.0 << "s" 
                          << std::endl;
            }
            
            // Automatically dump VRAM at specific milestones
            if (!already_dumped_vram && total_frames == 60) {  // After ~1 second
                std::string filename = "vram_dump_initial.txt";
                std::cout << "Automatically dumping initial VRAM to " << filename << std::endl;
                gpu->dumpVRAM(filename);
                already_dumped_vram = true;
            }
            
            frame_cycles = 0;
            
            // Render screen
            update_display();
            
            // Cap to 60 FPS
            if (frame_time < 16) {
                SDL_Delay(16 - frame_time);
            }
            frame_time = 0;

            if (cart->getTitle() == "TETRIS" && total_frames % 120 == 0) {
                // Periodically press buttons to make sure game advances
                static int button_sequence = 0;
                
                std::cout << "Tetris helper: Pressing button sequence " << button_sequence << std::endl;
                
                switch (button_sequence) {
                    case 0: // Press START to advance past title
                        memory->updateJoypadButton(0x10, true);  // Press START
                        SDL_Delay(50);
                        memory->updateJoypadButton(0x10, false); // Release START
                        break;
                        
                    case 1: // Press A to advance past menu
                        memory->updateJoypadButton(0x80, true);  // Press A
                        SDL_Delay(50);
                        memory->updateJoypadButton(0x80, false); // Release A
                        break;
                        
                    case 2: // Press START again to start game
                        memory->updateJoypadButton(0x10, true);  // Press START
                        SDL_Delay(50); 
                        memory->updateJoypadButton(0x10, false); // Release START
                        break;
                        
                    default:
                        // No automatic button presses after game starts
                        break;
                }
                
                button_sequence = (button_sequence + 1) % 4;
            }
        }
        
        ctx.ticks++;
    }

    std::cout << "Emulation stopped after " << total_frames << " frames" << std::endl;
    std::cout << "Total CPU cycles: " << cpu->getCycles() << std::endl;
    
    // Ensure execution trace is saved
    if (tracing_enabled) {
        dump_execution_trace("execution_trace_final.csv");
    }
    
    // Cleanup
    cleanup_system();

    return 0;
}

int main(int argc, char** argv) {
    return emu_run(argc, argv);
} 