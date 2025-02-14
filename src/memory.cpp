#include "memory.hpp"

MemoryBus::MemoryBus(Cartridge& cart) : cartridge(cart), ie_register(0) {
    // Initialize all memory regions to 0
    vram.fill(0);
    wram.fill(0);
    oam.fill(0);
    io_regs.fill(0);
    hram.fill(0);
}

uint8_t MemoryBus::read(uint16_t addr) const {
    // ROM banks (0x0000-0x7FFF)
    if (addr <= 0x7FFF) {
        return cartridge.read(addr);
    }
    // VRAM (0x8000-0x9FFF)
    else if (isInRange(addr, 0x8000, 0x9FFF)) {
        return vram[addr - 0x8000];
    }
    // External RAM (0xA000-0xBFFF)
    else if (isInRange(addr, 0xA000, 0xBFFF)) {
        return cartridge.read(addr);
    }
    // WRAM (0xC000-0xDFFF)
    else if (isInRange(addr, 0xC000, 0xDFFF)) {
        return wram[addr - 0xC000];
    }
    // Prohibited areas
    else if (isProhibitedRange(addr)) {
        return 0xFF; // Usually returns 0xFF on real hardware
    }
    // OAM (0xFE00-0xFE9F)
    else if (isInRange(addr, 0xFE00, 0xFE9F)) {
        return oam[addr - 0xFE00];
    }
    // I/O registers (0xFF00-0xFF7F)
    else if (isInRange(addr, 0xFF00, 0xFF7F)) {
        return io_regs[addr - 0xFF00];
    }
    // HRAM (0xFF80-0xFFFE)
    else if (isInRange(addr, 0xFF80, 0xFFFE)) {
        return hram[addr - 0xFF80];
    }
    // IE Register (0xFFFF)
    else if (addr == 0xFFFF) {
        return ie_register;
    }
    
    return 0xFF; // Default return value for unmapped memory
}

void MemoryBus::write(uint16_t addr, uint8_t value) {
    // ROM banks (0x0000-0x7FFF)
    if (addr <= 0x7FFF) {
        cartridge.write(addr, value);
    }
    // VRAM (0x8000-0x9FFF)
    else if (isInRange(addr, 0x8000, 0x9FFF)) {
        vram[addr - 0x8000] = value;
    }
    // External RAM (0xA000-0xBFFF)
    else if (isInRange(addr, 0xA000, 0xBFFF)) {
        cartridge.write(addr, value);
    }
    // WRAM (0xC000-0xDFFF)
    else if (isInRange(addr, 0xC000, 0xDFFF)) {
        wram[addr - 0xC000] = value;
    }
    // Ignore writes to prohibited areas
    else if (isProhibitedRange(addr)) {
        return;
    }
    // OAM (0xFE00-0xFE9F)
    else if (isInRange(addr, 0xFE00, 0xFE9F)) {
        oam[addr - 0xFE00] = value;
    }
    // I/O registers (0xFF00-0xFF7F)
    else if (isInRange(addr, 0xFF00, 0xFF7F)) {
        io_regs[addr - 0xFF00] = value;
    }
    // HRAM (0xFF80-0xFFFE)
    else if (isInRange(addr, 0xFF80, 0xFFFE)) {
        hram[addr - 0xFF80] = value;
    }
    // IE Register (0xFFFF)
    else if (addr == 0xFFFF) {
        ie_register = value;
    }
}
