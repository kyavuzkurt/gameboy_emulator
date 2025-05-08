#pragma once
#include "cartridge.hpp"
#include <array>
#include <cstdint>

class Timer; // Forward declaration
class GPU;   // Forward declaration for GPU class

class MemoryBus {
    public:
        explicit MemoryBus(Cartridge& cart, Timer& timer);

        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);
        void write16(uint16_t address, uint16_t value);
        uint16_t read16(uint16_t address);
        
        // Add GPU setter method
        void setGPU(GPU* gpu_ptr);

        // Add accessor methods for joypad state
        uint8_t getJoypadState() const { return joypad_state; }
        void setJoypadState(uint8_t state) { joypad_state = state; }
        
        // Method to update joypad button state and trigger interrupt if needed
        void updateJoypadButton(uint8_t button_mask, bool pressed);

    private:
        // Helper functions to check memory ranges
        bool isInRange(uint16_t addr, uint16_t start, uint16_t end) const {
            return addr >= start && addr <= end;
        }

        bool isProhibitedRange(uint16_t addr) const {
            // Echo RAM (E000-FDFF) and unused memory (FEA0-FEFF)
            return (addr >= 0xE000 && addr <= 0xFDFF) ||
                   (addr >= 0xFEA0 && addr <= 0xFEFF);
        }
        
        // Added method for DMA transfers
        void performDMATransfer(uint8_t value);
        
        // Memory regions 
        std::array<uint8_t, 0x2000> vram;     // 8KB Video RAM (0x8000-0x9FFF)
        std::array<uint8_t, 0x2000> wram;     // 8KB Work RAM (0xC000-0xDFFF)
        std::array<uint8_t, 0xA0> oam;        // 160B Object Attribute Memory (0xFE00-0xFE9F)
        std::array<uint8_t, 0x80> io_regs;    // 128B I/O Registers (0xFF00-0xFF7F)
        std::array<uint8_t, 0x7F> hram;       // 127B High RAM (0xFF80-0xFFFE)
        uint8_t ie_register;                   // Interrupt Enable Register (0xFFFF)
        
        Cartridge& cartridge;
        Timer& timer;                          // Reference to timer component
        GPU* gpu;                              // Pointer to GPU component
        
        // Debugging counters
        mutable uint32_t vram_write_counter = 0;
        mutable uint32_t write_counter = 0;

        // Joypad state (0xFF00)
        uint8_t joypad_state = 0xFF;  // All buttons released
        uint8_t joypad_select = 0xF0; // Default select value
};