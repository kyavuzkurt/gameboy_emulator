#pragma once
#include "cartridge.hpp"
#include <array>
#include <cstdint>

class MemoryBus {
    public:
        explicit MemoryBus(Cartridge& cart);

        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);

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
        
        // Memory regions 
        std::array<uint8_t, 0x2000> vram;     // 8KB Video RAM (0x8000-0x9FFF)
        std::array<uint8_t, 0x2000> wram;     // 8KB Work RAM (0xC000-0xDFFF)
        std::array<uint8_t, 0xA0> oam;        // 160B Object Attribute Memory (0xFE00-0xFE9F)
        std::array<uint8_t, 0x80> io_regs;    // 128B I/O Registers (0xFF00-0xFF7F)
        std::array<uint8_t, 0x7F> hram;       // 127B High RAM (0xFF80-0xFFFE)
        uint8_t ie_register;                   // Interrupt Enable Register (0xFFFF)
        
        Cartridge& cartridge;
};