#include "memory.hpp"
#include "timer.hpp"
#include "gpu.hpp"
#include <iostream>
#include <iomanip>

// Define interrupt constants
constexpr uint8_t INT_VBLANK = 0x01;
constexpr uint8_t INT_LCD_STAT = 0x02;
constexpr uint8_t INT_TIMER = 0x04;
constexpr uint8_t INT_SERIAL = 0x08;
constexpr uint8_t INT_JOYPAD = 0x10;

// I/O Register addresses
constexpr uint16_t IO_REGISTERS_START = 0xFF00;
constexpr uint16_t IO_REGISTERS_END = 0xFF7F;

// Memory-mapped I/O registers
constexpr uint16_t P1_REGISTER = 0xFF00;    // Joypad
constexpr uint16_t SB_REGISTER = 0xFF01;    // Serial transfer data
constexpr uint16_t SC_REGISTER = 0xFF02;    // Serial transfer control
constexpr uint16_t DIV_REGISTER = 0xFF04;   // Divider register
constexpr uint16_t TIMA_REGISTER = 0xFF05;  // Timer counter
constexpr uint16_t TMA_REGISTER = 0xFF06;   // Timer modulo
constexpr uint16_t TAC_REGISTER = 0xFF07;   // Timer control
constexpr uint16_t IF_REGISTER = 0xFF0F;    // Interrupt Flag

// Sound registers
constexpr uint16_t NR10_REGISTER = 0xFF10;  // Sound channel 1 sweep
constexpr uint16_t NR11_REGISTER = 0xFF11;  // Sound channel 1 length/wave duty
constexpr uint16_t NR12_REGISTER = 0xFF12;  // Sound channel 1 volume envelope
constexpr uint16_t NR13_REGISTER = 0xFF13;  // Sound channel 1 frequency low
constexpr uint16_t NR14_REGISTER = 0xFF14;  // Sound channel 1 frequency high
constexpr uint16_t NR21_REGISTER = 0xFF16;  // Sound channel 2 length/wave duty
constexpr uint16_t NR22_REGISTER = 0xFF17;  // Sound channel 2 volume envelope
constexpr uint16_t NR23_REGISTER = 0xFF18;  // Sound channel 2 frequency low
constexpr uint16_t NR24_REGISTER = 0xFF19;  // Sound channel 2 frequency high
constexpr uint16_t NR30_REGISTER = 0xFF1A;  // Sound channel 3 DAC enable
constexpr uint16_t NR31_REGISTER = 0xFF1B;  // Sound channel 3 length
constexpr uint16_t NR32_REGISTER = 0xFF1C;  // Sound channel 3 output level
constexpr uint16_t NR33_REGISTER = 0xFF1D;  // Sound channel 3 frequency low
constexpr uint16_t NR34_REGISTER = 0xFF1E;  // Sound channel 3 frequency high
constexpr uint16_t NR41_REGISTER = 0xFF20;  // Sound channel 4 length
constexpr uint16_t NR42_REGISTER = 0xFF21;  // Sound channel 4 volume envelope
constexpr uint16_t NR43_REGISTER = 0xFF22;  // Sound channel 4 polynomial counter
constexpr uint16_t NR44_REGISTER = 0xFF23;  // Sound channel 4 counter/consecutive
constexpr uint16_t NR50_REGISTER = 0xFF24;  // Sound control: SO1 volume and Vin->SO1 enable
constexpr uint16_t NR51_REGISTER = 0xFF25;  // Sound control: Selection of sound output terminal
constexpr uint16_t NR52_REGISTER = 0xFF26;  // Sound control: Sound on/off

// LCD registers
constexpr uint16_t LCDC_REG = 0xFF40;  // LCD Control
constexpr uint16_t STAT_REG = 0xFF41;  // LCD Status
constexpr uint16_t SCY_REGISTER = 0xFF42;   // Scroll Y
constexpr uint16_t SCX_REGISTER = 0xFF43;   // Scroll X
constexpr uint16_t LY_REGISTER = 0xFF44;    // LCD Y-Coordinate (read-only)
constexpr uint16_t LYC_REGISTER = 0xFF45;   // LY Compare
constexpr uint16_t DMA_REG = 0xFF46;   // DMA Transfer
constexpr uint16_t BGP_REG = 0xFF47;   // BG Palette Data
constexpr uint16_t OBP0_REG = 0xFF48;  // Object Palette 0 Data
constexpr uint16_t OBP1_REG = 0xFF49;  // Object Palette 1 Data
constexpr uint16_t WY_REGISTER = 0xFF4A;    // Window Y Position
constexpr uint16_t WX_REGISTER = 0xFF4B;    // Window X Position

// High RAM
constexpr uint16_t HRAM_START = 0xFF80;
constexpr uint16_t HRAM_END = 0xFFFE;

// Interrupt Enable register
constexpr uint16_t IE_REGISTER = 0xFFFF;

MemoryBus::MemoryBus(Cartridge& cart, Timer& timer) : cartridge(cart), timer(timer), gpu(nullptr), ie_register(0), joypad_state(0xFF), joypad_select(0xF0) {
    // Initialize all memory regions to 0
    vram.fill(0);
    wram.fill(0);
    oam.fill(0);
    io_regs.fill(0);
    hram.fill(0);
    
    // Initialize some registers with specific values
    io_regs[P1_REGISTER - IO_REGISTERS_START] = 0xCF;  // Input register (unused inputs pulled high)
    io_regs[DIV_REGISTER - IO_REGISTERS_START] = 0x18; // DIV register starts at random value
    io_regs[TIMA_REGISTER - IO_REGISTERS_START] = 0x00;
    io_regs[TMA_REGISTER - IO_REGISTERS_START] = 0x00;
    io_regs[TAC_REGISTER - IO_REGISTERS_START] = 0xF8;
    io_regs[IF_REGISTER - IO_REGISTERS_START] = 0xE1;
    
    // LCD initialization
    io_regs[LCDC_REG - IO_REGISTERS_START] = 0x91;
    io_regs[STAT_REG - IO_REGISTERS_START] = 0x85;
    io_regs[SCY_REGISTER - IO_REGISTERS_START] = 0x00;
    io_regs[SCX_REGISTER - IO_REGISTERS_START] = 0x00;
    io_regs[LY_REGISTER - IO_REGISTERS_START] = 0x00;
    io_regs[LYC_REGISTER - IO_REGISTERS_START] = 0x00;
    io_regs[BGP_REG - IO_REGISTERS_START] = 0xFC;
    io_regs[OBP0_REG - IO_REGISTERS_START] = 0x00;
    io_regs[OBP1_REG - IO_REGISTERS_START] = 0x00;
    io_regs[WY_REGISTER - IO_REGISTERS_START] = 0x00;
    io_regs[WX_REGISTER - IO_REGISTERS_START] = 0x00;
    
    // Initialize special addresses
    // Add a RET instruction (0xC9) at address 0xFFB6 for Tetris compatibility
    hram[0xFFB6 - 0xFF80] = 0xC9;
    
    // Initialize counters for debugging
    vram_write_counter = 0;
    write_counter = 0;
    
    std::cout << "MemoryBus initialized" << std::endl;
    std::cout << "Initialized address 0xFFB6 with RET instruction (0xC9) for Tetris compatibility" << std::endl;
}

void MemoryBus::setGPU(GPU* gpu_ptr) {
    gpu = gpu_ptr;
    
    // Force the GPU to check VRAM data immediately
    if (gpu != nullptr) {
        std::cout << "Forcing GPU to check VRAM data..." << std::endl;
        gpu->forceVRAMCheck();
    }
}

uint8_t MemoryBus::read(uint16_t addr) const {
    // ROM bank 0 & switchable ROM bank (handled by cartridge)
    if (addr < 0x8000) {
        return cartridge.read(addr);
    }
    // VRAM
    else if (isInRange(addr, 0x8000, 0x9FFF)) {
        // Check if VRAM is accessible - it's only inaccessible during pixel transfer (Mode 3)
        if (gpu && gpu->getCurrentMode() == LCDMode::TRANSFER) {
            // Debug: Log the first 10 blocked VRAM reads
            static int vram_read_blocked_count = 0;
            if (vram_read_blocked_count < 10) {
                std::cout << "VRAM read blocked (Mode 3) - addr: 0x" << std::hex << addr << std::dec << std::endl;
                vram_read_blocked_count++;
            }
            
            // IMPORTANT: For debugging - always allow VRAM access regardless of LCD mode
            // This bypass helps diagnose VRAM content issues
            return vram[addr - 0x8000];
            
            // During Mode 3 (pixel transfer), VRAM access returns 0xFF
            //return 0xFF;
        }
        
        return vram[addr - 0x8000];
    }
    // External RAM (handled by cartridge)
    else if (isInRange(addr, 0xA000, 0xBFFF)) {
        return cartridge.read(addr);
    }
    // WRAM
    else if (isInRange(addr, 0xC000, 0xDFFF)) {
        return wram[addr - 0xC000];
    }
    // Echo RAM - mirrors WRAM
    else if (isInRange(addr, 0xE000, 0xFDFF)) {
        return wram[addr - 0xE000];
    }
    // OAM
    else if (isInRange(addr, 0xFE00, 0xFE9F)) {
        // Check if OAM is accessible - it's inaccessible during OAM scan (Mode 2) and pixel transfer (Mode 3)
        if (gpu && (gpu->getCurrentMode() == LCDMode::OAM || 
                   gpu->getCurrentMode() == LCDMode::TRANSFER)) {
            // Debug: Log the first 10 blocked OAM reads
            static int oam_read_blocked_count = 0;
            if (oam_read_blocked_count < 10) {
                std::cout << "OAM read blocked (Mode " << (gpu->getCurrentMode() == LCDMode::OAM ? "2" : "3") 
                          << ") - addr: 0x" << std::hex << addr << std::dec << std::endl;
                oam_read_blocked_count++;
            }
            
            // IMPORTANT: For debugging - allow OAM access regardless of LCD mode
            return oam[addr - 0xFE00];
            
            // During Modes 2-3, OAM access returns 0xFF
            //return 0xFF;
        }
        
        return oam[addr - 0xFE00];
    }
    // Unused memory (returns 0xFF)
    else if (isInRange(addr, 0xFEA0, 0xFEFF)) {
        return 0xFF;
    }
    // I/O Registers
    else if (isInRange(addr, IO_REGISTERS_START, IO_REGISTERS_END)) {
        // Special handling for timer registers
        if (isInRange(addr, DIV_REGISTER, TAC_REGISTER)) {
            return timer.readRegister(addr);
        }
        // Handle joypad register (0xFF00)
        if (addr == P1_REGISTER) {
            uint8_t result = joypad_select & 0xF0; // Upper bits from select
            
            // Check which buttons are selected (by looking at the upper bits written to 0xFF00)
            if ((joypad_select & 0x20) == 0) {
                // Button keys selected (bit 5 = 0)
                result |= ((joypad_state >> 4) & 0x0F); // Return A, B, Select, Start
            } else if ((joypad_select & 0x10) == 0) {
                // Direction keys selected (bit 4 = 0)
                result |= (joypad_state & 0x0F); // Return Right, Left, Up, Down
            } else {
                // No keys selected, return all high
                result |= 0x0F;
            }
            
            return result;
        }
        
        // LY register (current scanline) - gets its value from the GPU
        if (addr == LY_REGISTER && gpu != nullptr) {
            // Let GPU provide the current scanline
            return io_regs[LY_REGISTER - IO_REGISTERS_START];
        }
        
        // Normal I/O register
        return io_regs[addr - IO_REGISTERS_START];
    }
    // High RAM
    else if (isInRange(addr, HRAM_START, HRAM_END)) {
        return hram[addr - HRAM_START];
    }
    // Interrupt Enable Register
    else if (addr == IE_REGISTER) {
        return ie_register;
    }
    // Default case (shouldn't happen)
    else {
        std::cerr << "WARNING: Reading from unmapped memory address: " << std::hex << addr << std::endl;
        return 0xFF;
    }
}

void MemoryBus::write(uint16_t addr, uint8_t value) {
    write_counter++;
    
    // ROM bank 0 & switchable ROM bank (handled by cartridge)
    if (addr < 0x8000) {
        cartridge.write(addr, value);
    }
    // VRAM
    else if (isInRange(addr, 0x8000, 0x9FFF)) {
        vram_write_counter++;
        
        // Check if VRAM is accessible - it's only inaccessible during pixel transfer (Mode 3)
        if (gpu && gpu->getCurrentMode() == LCDMode::TRANSFER) {
            // DEBUGGING: Temporarily allow all VRAM writes even during Mode 3 to verify game data
            // Comment out the warning and allow the write to proceed
            std::cout << "VRAM write during Mode 3 (allowed for debugging) - addr: 0x" << std::hex << addr 
                      << ", value: 0x" << static_cast<int>(value) << std::dec << std::endl;
            
            // Allow the write to go through during debugging
            vram[addr - 0x8000] = value;
            
            // Log more detailed information for the first 5 writes
            static int detailed_vram_writes = 0;
            if (detailed_vram_writes < 5) {
                std::cout << "DETAILED VRAM WRITE: address 0x" << std::hex << addr 
                          << " (offset 0x" << (addr - 0x8000) << ")" << std::endl;
                std::cout << "  - Value: 0x" << static_cast<int>(value) << std::dec << std::endl;
                std::cout << "  - Is tile data: " << (addr < 0x9800 ? "YES" : "NO") << std::endl;
                std::cout << "  - Is tilemap: " << (addr >= 0x9800 ? "YES" : "NO") << std::endl;
                detailed_vram_writes++;
            }
            
            // Normal behavior would be to return without writing
            //return;
        }
        
        // Debug: Log all VRAM writes to the background tiles memory (0x8000-0x97FF)
        if (addr >= 0x8000 && addr <= 0x97FF) {
            std::cout << "VRAM TILE WRITE: addr=0x" << std::hex << addr 
                      << ", value=0x" << static_cast<int>(value) << std::dec << std::endl;
        }
        
        // Debug: Log all VRAM writes to the background map (0x9800-0x9BFF)
        if (addr >= 0x9800 && addr <= 0x9BFF) {
            std::cout << "VRAM MAP WRITE: addr=0x" << std::hex << addr 
                      << ", value=0x" << static_cast<int>(value) << std::dec << std::endl;
        }
        
        // Debug: Log first 100 VRAM writes
        if (vram_write_counter <= 100) {
            std::cout << "VRAM write #" << vram_write_counter << " - addr: 0x" << std::hex << addr 
                      << ", value: 0x" << static_cast<int>(value) << std::dec << std::endl;
        }
        
        // Log periodic writes to see if VRAM writes continue
        if (vram_write_counter % 1000 == 0) {
            std::cout << "VRAM write #" << vram_write_counter << " - addr: 0x" << std::hex << addr 
                      << ", value: 0x" << static_cast<int>(value) << std::dec << std::endl;
        }
        
        vram[addr - 0x8000] = value;
    }
    // External RAM (handled by cartridge)
    else if (isInRange(addr, 0xA000, 0xBFFF)) {
        cartridge.write(addr, value);
    }
    // WRAM
    else if (isInRange(addr, 0xC000, 0xDFFF)) {
        wram[addr - 0xC000] = value;
    }
    // Echo RAM (mirrors to WRAM)
    else if (isInRange(addr, 0xE000, 0xFDFF)) {
        wram[addr - 0xE000] = value;
    }
    // OAM
    else if (isInRange(addr, 0xFE00, 0xFE9F)) {
        // Check if OAM is accessible - it's inaccessible during OAM scan (Mode 2) and pixel transfer (Mode 3)
        if (gpu && (gpu->getCurrentMode() == LCDMode::OAM || 
                   gpu->getCurrentMode() == LCDMode::TRANSFER)) {
            // DEBUGGING: Temporarily allow OAM writes during restricted modes
            std::cout << "OAM write during restricted mode (allowed for debugging) - addr: 0x" << std::hex << addr 
                      << ", value: 0x" << static_cast<int>(value) << std::dec << std::endl;
            
            // Allow the write to proceed
            oam[addr - 0xFE00] = value;
            
            // Normal behavior would block the write
            //return;
        }
        
        oam[addr - 0xFE00] = value;
    }
    // Prohibited memory area (writes are ignored)
    else if (isInRange(addr, 0xFEA0, 0xFEFF)) {
        // Writes to prohibited range are ignored
        return;
    }
    // I/O Registers
    else if (isInRange(addr, IO_REGISTERS_START, IO_REGISTERS_END)) {
        // Additional debug for important I/O registers
        if (addr == LCDC_REG) {
            std::cout << "LCD Control write: 0x" << std::hex << static_cast<int>(value) << std::dec 
                      << " (LCD " << ((value & 0x80) ? "ON" : "OFF") 
                      << ", BG " << ((value & 0x01) ? "ON" : "OFF") 
                      << ", Sprites " << ((value & 0x02) ? "ON" : "OFF") << ")" << std::endl;
        }
        else if (addr == DMA_REG) {
            std::cout << "DMA Transfer initiated from: 0x" << std::hex << (value * 0x100) << std::dec << std::endl;
        }
        else if (addr == BGP_REG) {
            std::cout << "Background Palette set: 0x" << std::hex << static_cast<int>(value) << std::dec << std::endl;
        }
        else if (addr == OBP0_REG || addr == OBP1_REG) {
            std::cout << "Sprite Palette " << ((addr == OBP0_REG) ? "0" : "1") << " set: 0x" << std::hex << static_cast<int>(value) << std::dec << std::endl;
        }
        
        // Special handling for timer registers
        if (isInRange(addr, DIV_REGISTER, TAC_REGISTER)) {
            timer.writeRegister(addr, value);
            
            // Also store the value in our I/O registers array for consistency
            if (addr != DIV_REGISTER) { // DIV always reads as 0 after writing
                io_regs[addr - IO_REGISTERS_START] = value;
            } else {
                io_regs[addr - IO_REGISTERS_START] = 0;
            }
            return;
        }
        
        // Handle joypad register (0xFF00)
        if (addr == P1_REGISTER) {
            // Only bits 4-5 are writable (select bits)
            joypad_select = value & 0x30;
            io_regs[P1_REGISTER - IO_REGISTERS_START] = value;
            return;
        }
        
        // Handle DMA transfers (0xFF46)
        if (addr == DMA_REG) {
            performDMATransfer(value);
            io_regs[DMA_REG - IO_REGISTERS_START] = value;
            return;
        }
        
        // Handle LY register (0xFF44) - read-only, writes reset to 0
        if (addr == LY_REGISTER) {
            io_regs[LY_REGISTER - IO_REGISTERS_START] = 0;
            return;
        }
        
        // Store in I/O registers array
        io_regs[addr - IO_REGISTERS_START] = value;
        
        // Handle other special registers that need additional processing
        if (gpu) {
            // LCDC changes might require GPU state updates
            if (addr == LCDC_REG) {
                // Check if LCD is being turned on or off (bit 7)
                bool lcd_enabled = (value & 0x80) != 0;
                bool was_enabled = (io_regs[LCDC_REG - IO_REGISTERS_START] & 0x80) != 0;
                
                if (!was_enabled && lcd_enabled) {
                    // LCD being turned ON - reset PPU state
                    io_regs[LY_REGISTER - IO_REGISTERS_START] = 0;
                }
            }
        }
    }
    // High RAM
    else if (isInRange(addr, HRAM_START, HRAM_END)) {
        hram[addr - HRAM_START] = value;
    }
    // Interrupt Enable Register
    else if (addr == IE_REGISTER) {
        ie_register = value;
    }
    // Default case (shouldn't happen)
    else {
        std::cerr << "WARNING: Writing to unmapped memory address: " << std::hex << addr << " value: " << static_cast<int>(value) << std::endl;
    }
}

void MemoryBus::write16(uint16_t address, uint16_t value) {
    write(address, value & 0xFF);
    write(address + 1, value >> 8);
}

uint16_t MemoryBus::read16(uint16_t address) {
    return read(address) | (read(address + 1) << 8);
}

void MemoryBus::performDMATransfer(uint8_t value) {
    // DMA transfers copy data from XX00-XX9F to OAM (FE00-FE9F)
    // Where XX is the value written to DMA register (e.g., 30 = 3000-309F)
    uint16_t source = value << 8; // multiply by 0x100
    
    // Perform the transfer (should take 160 cycles in reality, but we do it instantly here)
    for (uint8_t i = 0; i < 0xA0; i++) {
        // Read source byte - we use the regular read function which respects memory mapping
        uint8_t data = read(source + i);
        
        // Direct OAM access, bypassing restrictions because DMA has higher priority than PPU
        oam[i] = data;
    }
}

void MemoryBus::updateJoypadButton(uint8_t button_mask, bool pressed) {
    if (pressed) {
        // When a button is pressed, the corresponding bit is CLEARED
        joypad_state &= ~button_mask;
    } else {
        // When a button is released, the corresponding bit is SET
        joypad_state |= button_mask;
    }
    
    // Check if any button was just pressed (transition from 1 to 0)
    // and the joypad interrupt is enabled
    if (pressed) {
        // Set the joypad interrupt flag (bit 4)
        uint8_t if_value = read(IF_REGISTER);
        write(IF_REGISTER, if_value | INT_JOYPAD);
    }
}
