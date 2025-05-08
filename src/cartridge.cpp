#include "cartridge.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <cstring>
#include <sstream>
#include <chrono>
#include <ctime>

// Static lookup table for new license codes
static const std::unordered_map<uint16_t, std::string> NEW_LICENSE_CODES = {
    {0x00, "None"},
    {0x01, "Nintendo Research & Development 1"},
    {0x08, "Capcom"},
    {0x13, "Electronic Arts"},
    {0x18, "Hudson Soft"},
    {0x19, "B-AI"},
    {0x20, "KSS"},
    {0x22, "Planning Office WADA"},
    {0x24, "PCM Complete"},
    {0x25, "San-X"},
    {0x28, "Kemco"},
    {0x29, "SETA Corporation"},
    {0x30, "Viacom"},
    {0x31, "Nintendo"},
    {0x32, "Bandai"},
    {0x33, "Ocean Software/Acclaim Entertainment"},
    {0x34, "Konami"},
    {0x35, "HectorSoft"},
    {0x37, "Taito"},
    {0x38, "Hudson Soft"},
    {0x39, "Banpresto"},
    {0x41, "Ubi Soft"},
    {0x42, "Atlus"},
    {0x44, "Malibu Interactive"},
    {0x46, "Angel"},
    {0x47, "Bullet-Proof Software"},
    {0x49, "Irem"},
    {0x50, "Absolute"},
    {0x51, "Acclaim Entertainment"},
    {0x52, "Activision"},
    {0x53, "Sammy USA Corporation"},
    {0x54, "Konami"},
    {0x55, "Hi Tech Expressions"},
    {0x56, "LJN"},
    {0x57, "Matchbox"},
    {0x58, "Mattel"},
    {0x59, "Milton Bradley Company"},
    {0x60, "Titus Interactive"},
    {0x61, "Virgin Games Ltd."},
    {0x64, "Lucasfilm Games"},
    {0x67, "Ocean Software"},
    {0x69, "Electronic Arts"},
    {0x70, "Infogrames"},
    {0x71, "Interplay Entertainment"},
    {0x72, "Broderbund"},
    {0x73, "Sculptured Software"},
    {0x75, "The Sales Curve Limited"},
    {0x78, "THQ"},
    {0x79, "Accolade"},
    {0x80, "Misawa Entertainment"},
    {0x83, "lozc"},
    {0x86, "Tokuma Shoten"},
    {0x87, "Tsukuda Original"},
    {0x91, "Chunsoft Co."},
    {0x92, "Video System"},
    {0x93, "Ocean Software/Acclaim Entertainment"},
    {0x95, "Varie"},
    {0x96, "Yonezawa/s'pal"},
    {0x97, "Kaneko"},
    {0x99, "Pack-In-Video"},
    {0x9A, "Bottom Up"},
    {0xA4, "Konami (Yu-Gi-Oh!)"}
};

// Static lookup table for cartridge types
static const std::unordered_map<uint8_t, std::string> CARTRIDGE_TYPES = {
    {0x00, "ROM ONLY"},
    {0x01, "MBC1"},
    {0x02, "MBC1+RAM"},
    {0x03, "MBC1+RAM+BATTERY"},
    {0x05, "MBC2"},
    {0x06, "MBC2+BATTERY"},
    {0x08, "ROM+RAM"},
    {0x09, "ROM+RAM+BATTERY"},
    {0x0B, "MMM01"},
    {0x0C, "MMM01+RAM"},
    {0x0D, "MMM01+RAM+BATTERY"},
    {0x0F, "MBC3+TIMER+BATTERY"},
    {0x10, "MBC3+TIMER+RAM+BATTERY"},
    {0x11, "MBC3"},
    {0x12, "MBC3+RAM"},
    {0x13, "MBC3+RAM+BATTERY"},
    {0x19, "MBC5"},
    {0x1A, "MBC5+RAM"},
    {0x1B, "MBC5+RAM+BATTERY"},
    {0x1C, "MBC5+RUMBLE"},
    {0x1D, "MBC5+RUMBLE+RAM"},
    {0x1E, "MBC5+RUMBLE+RAM+BATTERY"},
    {0x20, "MBC6"},
    {0x22, "MBC7+SENSOR+RUMBLE+RAM+BATTERY"},
    {0xFC, "POCKET CAMERA"},
    {0xFD, "BANDAI TAMA5"},
    {0xFE, "HuC3"},
    {0xFF, "HuC1+RAM+BATTERY"}
};

// RAM size lookup table (in bytes)
static const std::unordered_map<uint8_t, uint32_t> RAM_SIZES = {
    {0x00, 0},
    {0x01, 2 * 1024},          // 2KB
    {0x02, 8 * 1024},          // 8KB
    {0x03, 32 * 1024},         // 32KB (4 banks of 8KB)
    {0x04, 128 * 1024},        // 128KB (16 banks of 8KB)
    {0x05, 64 * 1024},         // 64KB (8 banks of 8KB)
};

// ROM size lookup table (in bytes)
static const std::unordered_map<uint8_t, uint32_t> ROM_SIZES = {
    {0x00, 32 * 1024},         // 32KB (2 banks of 16KB)
    {0x01, 64 * 1024},         // 64KB (4 banks of 16KB)
    {0x02, 128 * 1024},        // 128KB (8 banks of 16KB)
    {0x03, 256 * 1024},        // 256KB (16 banks of 16KB)
    {0x04, 512 * 1024},        // 512KB (32 banks of 16KB)
    {0x05, 1024 * 1024},       // 1MB (64 banks of 16KB)
    {0x06, 2 * 1024 * 1024},   // 2MB (128 banks of 16KB)
    {0x07, 4 * 1024 * 1024},   // 4MB (256 banks of 16KB)
    {0x08, 8 * 1024 * 1024},   // 8MB (512 banks of 16KB)
    // Additional sizes for 52 pin cartridges
    {0x52, 1179648},           // 1.1MB (72 banks of 16KB)
    {0x53, 1310720},           // 1.2MB (80 banks of 16KB)
    {0x54, 1572864},           // 1.5MB (96 banks of 16KB)
};

// ==============================================
// ROMOnly Implementation
// ==============================================

ROMOnly::ROMOnly(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram) 
    : rom(rom_data), ram(ram), ram_enabled(false) {
}

uint8_t ROMOnly::read(uint16_t addr) const {
    // ROM area (0x0000 - 0x7FFF)
    if (addr <= 0x7FFF) {
        if (addr < rom.size()) {
            return rom[addr];
        }
        return 0xFF;  // Reading beyond ROM size returns 0xFF
    }
    
    // RAM area (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled && addr - 0xA000 < ram.size()) {
            return ram[addr - 0xA000];
        }
        return 0xFF;  // Reading disabled RAM or out of bounds returns 0xFF
    }
    
    // Unmapped memory
    return 0xFF;
}

void ROMOnly::write(uint16_t addr, uint8_t value) {
    // ROM area (0x0000 - 0x7FFF) - Ignored in ROM-only carts
    if (addr <= 0x7FFF) {
        // Enable/disable RAM if address is in the right range (common for all MBCs)
        if (addr <= 0x1FFF) {
            ram_enabled = ((value & 0x0F) == 0x0A);
        }
        // Otherwise writes to ROM are ignored
    }
    
    // RAM area (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled && addr - 0xA000 < ram.size()) {
            ram[addr - 0xA000] = value;
        }
        // Writes to disabled RAM are ignored
    }
    
    // Unmapped memory - Ignored
}

// ==============================================
// MBC1 Implementation
// ==============================================

MBC1::MBC1(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery, bool is_multicart)
    : rom(rom_data), ram(ram), ram_enabled(false), rom_bank(1), ram_bank(0), 
      mode_select(false), battery(has_battery), multicart(is_multicart) {
}

uint8_t MBC1::read(uint16_t addr) const {
    // ROM Bank 0 (0x0000 - 0x3FFF)
    if (addr <= 0x3FFF) {
        uint32_t rom_addr;
        
        // In mode 1, apply the RAM bank register to upper bits
        if (mode_select) {
            if (multicart) {
                // MBC1M multicart: RAM bank selects which 256KB game to use
                rom_addr = (ram_bank << 18) | (addr & 0x3FFF);
            } else {
                // Standard MBC1: RAM bank provides upper bits of ROM bank
                rom_addr = (ram_bank << 19) | (addr & 0x3FFF);
            }
        } else {
            // Mode 0: Always use the first bank
            rom_addr = addr;
        }
        
        if (rom_addr < rom.size()) {
            return rom[rom_addr];
        }
        return 0xFF;
    }
    
    // ROM Bank 1-127 (0x4000 - 0x7FFF)
    else if (addr >= 0x4000 && addr <= 0x7FFF) {
        uint32_t bank_start = getRomBankStart();
        uint32_t rom_addr = bank_start + (addr - 0x4000);
        
        if (rom_addr < rom.size()) {
            return rom[rom_addr];
        }
        return 0xFF;
    }
    
    // RAM Banks (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled && !ram.empty()) {
            uint32_t ram_addr;
            
            // In mode 1, use the RAM bank register for RAM banking
            if (mode_select) {
                ram_addr = getRamBankStart() + (addr - 0xA000);
            } else {
                // Mode 0: Always use the first RAM bank
                ram_addr = addr - 0xA000;
            }
            
            if (ram_addr < ram.size()) {
                return ram[ram_addr];
            }
        }
        return 0xFF;
    }
    
    // Unmapped memory
    return 0xFF;
}

void MBC1::write(uint16_t addr, uint8_t value) {
    // RAM Enable (0x0000 - 0x1FFF)
    if (addr <= 0x1FFF) {
        ram_enabled = ((value & 0x0F) == 0x0A);
    }
    
    // ROM Bank Number (0x2000 - 0x3FFF)
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // Extract the lower 5 bits (bank number)
        rom_bank = value & 0x1F;
        
        // Bank 0 is translated to 1 for the switchable bank region
        if (rom_bank == 0) {
            rom_bank = 1;
        }
    }
    
    // RAM Bank Number/Upper ROM Bank bits (0x4000 - 0x5FFF)
    else if (addr >= 0x4000 && addr <= 0x5FFF) {
        // Extract the lower 2 bits
        ram_bank = value & 0x03;
    }
    
    // Banking Mode Select (0x6000 - 0x7FFF)
    else if (addr >= 0x6000 && addr <= 0x7FFF) {
        // Extract the lowest bit for mode select
        mode_select = (value & 0x01) != 0;
    }
    
    // RAM Banks (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled && !ram.empty()) {
            uint32_t ram_addr;
            
            // In mode 1, use the RAM bank register for RAM banking
            if (mode_select) {
                ram_addr = getRamBankStart() + (addr - 0xA000);
            } else {
                // Mode 0: Always use the first RAM bank
                ram_addr = addr - 0xA000;
            }
            
            if (ram_addr < ram.size()) {
                ram[ram_addr] = value;
            }
        }
    }
}

uint32_t MBC1::getRomBankStart() const {
    uint32_t bank;
    
    if (multicart) {
        // MBC1M multicart: RAM bank selects which 256KB game to use
        // Special case: banks 0x20, 0x40, 0x60 are not accessible in ROM bank 1-127 region
        // They're mapped to 0x21, 0x41, 0x61 instead
        uint8_t effective_rom_bank = rom_bank;
        
        // Ignore upper bit of ROM bank register in multicart
        effective_rom_bank &= 0x0F;
        
        bank = (ram_bank << 4 | effective_rom_bank);
        
        // If trying to address banks 0x10, 0x20, 0x30, increment by 1
        if (effective_rom_bank == 0) {
            bank += 1;
        }
    } else {
        // Standard MBC1: RAM bank may provide upper bits of ROM bank
        bank = (ram_bank << 5) | rom_bank;
        
        // Special case: banks 0x20, 0x40, 0x60 are not accessible in ROM bank 1-127 region
        // They're mapped to 0x21, 0x41, 0x61 instead
        if ((bank & 0x1F) == 0) {
            bank += 1;
        }
    }
    
    return bank * 0x4000; // Each bank is 16KB
}

uint32_t MBC1::getRamBankStart() const {
    // Only use RAM banking if in mode 1 and the cartridge has more than 8KB RAM
    if (mode_select && ram.size() > 0x2000) {
        return ram_bank * 0x2000; // Each RAM bank is 8KB
    }
    return 0;
}

bool MBC1::saveRAM(const std::string& save_path) const {
    if (!battery || ram.empty()) {
        return false;
    }
    
    std::ofstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << save_path << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(ram.data()), ram.size());
    
    std::cout << "Saved RAM to: " << save_path << " (" << ram.size() << " bytes)" << std::endl;
    return true;
}

bool MBC1::loadRAM(const std::string& save_path) {
    if (!battery || ram.empty()) {
        return false;
    }
    
    std::ifstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << save_path << std::endl;
        return false; // Not an error - might be first time running
    }
    
    file.read(reinterpret_cast<char*>(ram.data()), ram.size());
    
    std::cout << "Loaded RAM from: " << save_path << " (" 
              << file.gcount() << " of " << ram.size() << " bytes)" << std::endl;
    return true;
}

// ==============================================
// MBC2 Implementation
// ==============================================

MBC2::MBC2(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery)
    : rom(rom_data), ram(ram), ram_enabled(false), rom_bank(1), battery(has_battery) {
}

uint8_t MBC2::read(uint16_t addr) const {
    // ROM Bank 0 (0x0000 - 0x3FFF)
    if (addr <= 0x3FFF) {
        if (addr < rom.size()) {
            return rom[addr];
        }
        return 0xFF;
    }
    
    // ROM Bank 1-15 (0x4000 - 0x7FFF)
    else if (addr >= 0x4000 && addr <= 0x7FFF) {
        uint32_t rom_addr = (rom_bank * 0x4000) + (addr - 0x4000);
        
        if (rom_addr < rom.size()) {
            return rom[rom_addr];
        }
        return 0xFF;
    }
    
    // Built-in RAM (0xA000 - 0xA1FF)
    else if (addr >= 0xA000 && addr <= 0xA1FF) {
        if (ram_enabled) {
            // MBC2 has 512 x 4 bits of RAM (only lower 4 bits are used)
            return ram[addr - 0xA000] & 0x0F;
        }
        return 0xFF;
    }
    
    // Unmapped RAM (0xA200 - 0xBFFF) - Returns 0x00 per GB manual
    else if (addr >= 0xA200 && addr <= 0xBFFF) {
        return 0x00;
    }
    
    // Unmapped memory
    return 0xFF;
}

void MBC2::write(uint16_t addr, uint8_t value) {
    // RAM Enable / ROM Bank Number (0x0000 - 0x3FFF)
    if (addr <= 0x3FFF) {
        // The 8th bit of the address determines the register
        if (addr & 0x0100) {
            // ROM Bank Number (bit 8 of address is set)
            rom_bank = value & 0x0F;
            if (rom_bank == 0) {
                rom_bank = 1;  // Bank 0 is treated as 1
            }
        } else {
            // RAM Enable (bit 8 of address is clear)
            ram_enabled = ((value & 0x0F) == 0x0A);
        }
    }
    
    // RAM area (0xA000 - 0xA1FF)
    else if (addr >= 0xA000 && addr <= 0xA1FF) {
        if (ram_enabled) {
            // MBC2 has 512 x 4 bits of RAM (only lower 4 bits are used)
            ram[addr - 0xA000] = value & 0x0F;
        }
    }
    
    // Unmapped areas - writes ignored
}

bool MBC2::saveRAM(const std::string& save_path) const {
    if (!battery || ram.empty()) {
        return false;
    }
    
    std::ofstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << save_path << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(ram.data()), ram.size());
    
    std::cout << "Saved MBC2 RAM to: " << save_path << " (" << ram.size() << " bytes)" << std::endl;
    return true;
}

bool MBC2::loadRAM(const std::string& save_path) {
    if (!battery || ram.empty()) {
        return false;
    }
    
    std::ifstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << save_path << std::endl;
        return false; // Not an error - might be first time running
    }
    
    file.read(reinterpret_cast<char*>(ram.data()), ram.size());
    
    std::cout << "Loaded MBC2 RAM from: " << save_path << " (" 
              << file.gcount() << " of " << ram.size() << " bytes)" << std::endl;
    return true;
}

// ==============================================
// MBC3 Implementation
// ==============================================

MBC3::MBC3(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery, bool has_rtc)
    : rom(rom_data), ram(ram), ram_enabled(false), rom_bank(1), ram_bank(0), 
      battery(has_battery), rtc(has_rtc), rtc_latch(false) {
    
    // Initialize RTC registers
    rtc_s = 0;
    rtc_m = 0;
    rtc_h = 0;
    rtc_dl = 0;
    rtc_dh = 0;
    
    // Initialize latched RTC registers
    latch_rtc_s = 0;
    latch_rtc_m = 0;
    latch_rtc_h = 0;
    latch_rtc_dl = 0;
    latch_rtc_dh = 0;
    
    // Update RTC to current time if RTC is enabled
    if (rtc) {
        updateRTC();
    }
}

uint8_t MBC3::read(uint16_t addr) const {
    // ROM Bank 0 (0x0000 - 0x3FFF)
    if (addr <= 0x3FFF) {
        if (addr < rom.size()) {
            return rom[addr];
        }
        return 0xFF;
    }
    
    // ROM Bank 1-127 (0x4000 - 0x7FFF)
    else if (addr >= 0x4000 && addr <= 0x7FFF) {
        uint32_t rom_addr = (rom_bank * 0x4000) + (addr - 0x4000);
        
        if (rom_addr < rom.size()) {
            return rom[rom_addr];
        }
        return 0xFF;
    }
    
    // RAM Banks or RTC Registers (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled) {
            // RTC Register (0x08-0x0C)
            if (rtc && ram_bank >= 0x08 && ram_bank <= 0x0C) {
                switch (ram_bank) {
                    case 0x08: return latch_rtc_s;
                    case 0x09: return latch_rtc_m;
                    case 0x0A: return latch_rtc_h;
                    case 0x0B: return latch_rtc_dl;
                    case 0x0C: return latch_rtc_dh;
                    default: return 0xFF; // Should never happen
                }
            }
            // RAM Bank (0x00-0x07)
            else if (ram_bank <= 0x07 && !ram.empty()) {
                uint32_t ram_addr = (ram_bank * 0x2000) + (addr - 0xA000);
                
                if (ram_addr < ram.size()) {
                    return ram[ram_addr];
                }
            }
        }
        return 0xFF;
    }
    
    // Unmapped memory
    return 0xFF;
}

void MBC3::write(uint16_t addr, uint8_t value) {
    // RAM and Timer Enable (0x0000 - 0x1FFF)
    if (addr <= 0x1FFF) {
        ram_enabled = ((value & 0x0F) == 0x0A);
    }
    
    // ROM Bank Number (0x2000 - 0x3FFF)
    else if (addr >= 0x2000 && addr <= 0x3FFF) {
        // Extract the ROM bank number (7 bits)
        rom_bank = value & 0x7F;
        
        // Treat bank 0 as bank 1
        if (rom_bank == 0) {
            rom_bank = 1;
        }
    }
    
    // RAM Bank Number or RTC Register Select (0x4000 - 0x5FFF)
    else if (addr >= 0x4000 && addr <= 0x5FFF) {
        ram_bank = value;
    }
    
    // Latch Clock Data (0x6000 - 0x7FFF)
    else if (addr >= 0x6000 && addr <= 0x7FFF) {
        // RTC latch sequence: write 0x00 then 0x01 to latch the RTC data
        if (rtc) {
            if (value == 0x00 && !rtc_latch) {
                rtc_latch = true;
            }
            else if (value == 0x01 && rtc_latch) {
                // Latch the current RTC data
                latchRTC();
                rtc_latch = false;
            }
            else {
                rtc_latch = false;
            }
        }
    }
    
    // RAM Banks or RTC Registers (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled) {
            // RTC Register (0x08-0x0C)
            if (rtc && ram_bank >= 0x08 && ram_bank <= 0x0C) {
                switch (ram_bank) {
                    case 0x08:
                        rtc_s = value & 0x3F; // 0-59
                        break;
                    case 0x09:
                        rtc_m = value & 0x3F; // 0-59
                        break;
                    case 0x0A:
                        rtc_h = value & 0x1F; // 0-23
                        break;
                    case 0x0B:
                        rtc_dl = value; // 0-255 (lower 8 bits of day counter)
                        break;
                    case 0x0C:
                        rtc_dh = value & 0xC1; // Only bits 0, 6, 7 are used
                        break;
                }
            }
            // RAM Bank (0x00-0x07)
            else if (ram_bank <= 0x07 && !ram.empty()) {
                uint32_t ram_addr = (ram_bank * 0x2000) + (addr - 0xA000);
                
                if (ram_addr < ram.size()) {
                    ram[ram_addr] = value;
                }
            }
        }
    }
}

void MBC3::latchRTC() {
    // Update the RTC first
    if (!(rtc_dh & 0x40)) { // Halt flag not set
        updateRTC();
    }
    
    // Latch the current time
    latch_rtc_s = rtc_s;
    latch_rtc_m = rtc_m;
    latch_rtc_h = rtc_h;
    latch_rtc_dl = rtc_dl;
    latch_rtc_dh = rtc_dh;
}

void MBC3::updateRTC() {
    // Only update if the RTC is not halted
    if (rtc_dh & 0x40) {
        return; // RTC is halted
    }
    
    // Get current system time
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    std::tm* time_info = std::localtime(&now_time);
    
    // Update RTC registers
    rtc_s = time_info->tm_sec;
    rtc_m = time_info->tm_min;
    rtc_h = time_info->tm_hour;
    
    // Update day counter (we'd need to store the last sync time somewhere to do this accurately)
    // For now, we'll just use the day of year, which isn't ideal but works for demonstration
    uint16_t days = time_info->tm_yday;
    
    rtc_dl = days & 0xFF;     // Lower 8 bits of days
    
    // Update upper bit of day counter
    if (days > 255) {
        rtc_dh |= 0x01;       // Set bit 0 (day counter high bit)
    } else {
        rtc_dh &= ~0x01;      // Clear bit 0
    }
    
    // Day counter overflow (days > 511) would set the carry bit (bit 7)
    if (days > 511) {
        rtc_dh |= 0x80;       // Set bit 7 (carry flag)
    }
}

bool MBC3::saveRAM(const std::string& save_path) const {
    if (!battery || (ram.empty() && !rtc)) {
        return false;
    }
    
    std::ofstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << save_path << std::endl;
        return false;
    }
    
    // Save RAM if present
    if (!ram.empty()) {
        file.write(reinterpret_cast<const char*>(ram.data()), ram.size());
    }
    
    std::cout << "Saved MBC3 RAM to: " << save_path;
    if (!ram.empty()) {
        std::cout << " (" << ram.size() << " bytes)";
    }
    std::cout << std::endl;
    
    // Save RTC registers if RTC is enabled
    if (rtc) {
        saveRTC(save_path + ".rtc");
    }
    
    return true;
}

bool MBC3::loadRAM(const std::string& save_path) {
    if (!battery || (ram.empty() && !rtc)) {
        return false;
    }
    
    bool success = true;
    
    // Load RAM data if present
    if (!ram.empty()) {
        std::ifstream file(save_path, std::ios::binary);
        if (!file.is_open()) {
            std::cerr << "Failed to open save file: " << save_path << std::endl;
            success = false;
        } else {
            file.read(reinterpret_cast<char*>(ram.data()), ram.size());
            std::cout << "Loaded MBC3 RAM from: " << save_path 
                      << " (" << file.gcount() << " of " << ram.size() << " bytes)" << std::endl;
        }
    }
    
    // Load RTC registers if RTC is enabled
    if (rtc) {
        bool rtc_loaded = loadRTC(save_path + ".rtc");
        success = success && rtc_loaded;
    }
    
    return success;
}

bool MBC3::loadRTC(const std::string& save_path) {
    std::ifstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open RTC save file: " << save_path << std::endl;
        return false;
    }
    
    // Load the saved RTC state
    file.read(reinterpret_cast<char*>(&rtc_s), sizeof(rtc_s));
    file.read(reinterpret_cast<char*>(&rtc_m), sizeof(rtc_m));
    file.read(reinterpret_cast<char*>(&rtc_h), sizeof(rtc_h));
    file.read(reinterpret_cast<char*>(&rtc_dl), sizeof(rtc_dl));
    file.read(reinterpret_cast<char*>(&rtc_dh), sizeof(rtc_dh));
    
    // Load the saved system time
    std::time_t saved_time;
    file.read(reinterpret_cast<char*>(&saved_time), sizeof(saved_time));
    
    // Calculate elapsed time since last save if RTC is running
    if (!(rtc_dh & 0x40)) { // Not halted
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::time_t elapsed = now_time - saved_time;
        
        // Update RTC registers based on elapsed time
        uint32_t seconds = elapsed % 60;
        uint32_t minutes = (elapsed / 60) % 60;
        uint32_t hours = (elapsed / 3600) % 24;
        uint32_t days = elapsed / 86400; // seconds in a day
        
        // Add elapsed time to RTC values
        rtc_s = (rtc_s + seconds) % 60;
        
        minutes += (rtc_s + seconds) / 60;
        rtc_m = (rtc_m + minutes) % 60;
        
        hours += (rtc_m + minutes) / 60;
        rtc_h = (rtc_h + hours) % 24;
        
        days += (rtc_h + hours) / 24;
        
        // Update day counter
        uint16_t day_counter = rtc_dl;
        if (rtc_dh & 0x01) {
            day_counter |= 0x100; // Add the 9th bit
        }
        
        day_counter += days;
        
        // Check for day counter overflow (> 511 days)
        if (day_counter > 511) {
            rtc_dh |= 0x80; // Set day counter carry flag
            day_counter &= 0x1FF; // Keep only the lower 9 bits
        }
        
        // Update day counter registers
        rtc_dl = day_counter & 0xFF;
        if (day_counter & 0x100) {
            rtc_dh |= 0x01; // Set bit 0
        } else {
            rtc_dh &= ~0x01; // Clear bit 0
        }
    }
    
    // Latch the updated values
    latchRTC();
    
    std::cout << "Loaded MBC3 RTC state from: " << save_path << std::endl;
    return true;
}

void MBC3::saveRTC(const std::string& save_path) const {
    std::ofstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open RTC save file: " << save_path << std::endl;
        return;
    }
    
    // Save the current RTC state
    file.write(reinterpret_cast<const char*>(&rtc_s), sizeof(rtc_s));
    file.write(reinterpret_cast<const char*>(&rtc_m), sizeof(rtc_m));
    file.write(reinterpret_cast<const char*>(&rtc_h), sizeof(rtc_h));
    file.write(reinterpret_cast<const char*>(&rtc_dl), sizeof(rtc_dl));
    file.write(reinterpret_cast<const char*>(&rtc_dh), sizeof(rtc_dh));
    
    // Save the current system time for calculating elapsed time when loading
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    file.write(reinterpret_cast<const char*>(&now_time), sizeof(now_time));
    
    std::cout << "Saved MBC3 RTC state to: " << save_path << std::endl;
}

// ==============================================
// MBC5 Implementation
// ==============================================

MBC5::MBC5(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery, bool has_rumble)
    : rom(rom_data), ram(ram), ram_enabled(false), rom_bank(1), ram_bank(0), 
      battery(has_battery), rumble(has_rumble) {
}

uint8_t MBC5::read(uint16_t addr) const {
    // ROM Bank 0 (0x0000 - 0x3FFF)
    if (addr <= 0x3FFF) {
        if (addr < rom.size()) {
            return rom[addr];
        }
        return 0xFF;
    }
    
    // ROM Bank 1-511 (0x4000 - 0x7FFF)
    else if (addr >= 0x4000 && addr <= 0x7FFF) {
        uint32_t rom_addr = (rom_bank * 0x4000) + (addr - 0x4000);
        
        if (rom_addr < rom.size()) {
            return rom[rom_addr];
        }
        return 0xFF;
    }
    
    // RAM Banks (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled && !ram.empty()) {
            uint32_t ram_addr = (ram_bank * 0x2000) + (addr - 0xA000);
            
            if (ram_addr < ram.size()) {
                return ram[ram_addr];
            }
        }
        return 0xFF;
    }
    
    // Unmapped memory
    return 0xFF;
}

void MBC5::write(uint16_t addr, uint8_t value) {
    // RAM Enable (0x0000 - 0x1FFF)
    if (addr <= 0x1FFF) {
        ram_enabled = ((value & 0x0F) == 0x0A);
    }
    
    // ROM Bank Number Lower 8 bits (0x2000 - 0x2FFF)
    else if (addr >= 0x2000 && addr <= 0x2FFF) {
        // Set lower 8 bits of ROM bank number
        rom_bank = (rom_bank & 0x100) | value;
    }
    
    // ROM Bank Number 9th bit (0x3000 - 0x3FFF)
    else if (addr >= 0x3000 && addr <= 0x3FFF) {
        // Set 9th bit of ROM bank number
        rom_bank = (rom_bank & 0xFF) | ((value & 0x01) << 8);
    }
    
    // RAM Bank Number (0x4000 - 0x5FFF)
    else if (addr >= 0x4000 && addr <= 0x5FFF) {
        // In rumble cartridges, bit 3 controls the rumble motor
        if (rumble) {
            // Extract bits 0-2 for RAM bank selection
            ram_bank = value & 0x07;
            
            // Extract bit 3 for rumble motor
            // bool rumble_on = (value & 0x08) != 0;
            // Here you would control the rumble motor if implemented
        } else {
            // Extract bits 0-3 for RAM bank selection (up to 16 banks)
            ram_bank = value & 0x0F;
        }
    }
    
    // RAM Banks (0xA000 - 0xBFFF)
    else if (addr >= 0xA000 && addr <= 0xBFFF) {
        if (ram_enabled && !ram.empty()) {
            uint32_t ram_addr = (ram_bank * 0x2000) + (addr - 0xA000);
            
            if (ram_addr < ram.size()) {
                ram[ram_addr] = value;
            }
        }
    }
}

bool MBC5::saveRAM(const std::string& save_path) const {
    if (!battery || ram.empty()) {
        return false;
    }
    
    std::ofstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << save_path << std::endl;
        return false;
    }
    
    file.write(reinterpret_cast<const char*>(ram.data()), ram.size());
    
    std::cout << "Saved MBC5 RAM to: " << save_path << " (" << ram.size() << " bytes)" << std::endl;
    return true;
}

bool MBC5::loadRAM(const std::string& save_path) {
    if (!battery || ram.empty()) {
        return false;
    }
    
    std::ifstream file(save_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open save file: " << save_path << std::endl;
        return false; // Not an error - might be first time running
    }
    
    file.read(reinterpret_cast<char*>(ram.data()), ram.size());
    
    std::cout << "Loaded MBC5 RAM from: " << save_path << " (" 
              << file.gcount() << " of " << ram.size() << " bytes)" << std::endl;
    return true;
}

// ==============================================
// Cartridge Implementation
// ==============================================

std::string Cartridge::getPublisherName() const {
    auto it = NEW_LICENSE_CODES.find(header.oldLicenseCode);
    if (it != NEW_LICENSE_CODES.end()) {
        return it->second;
    }
    return "Unknown Publisher";
}

std::string Cartridge::getCartridgeTypeName() const {
    auto it = CARTRIDGE_TYPES.find(header.cartridgeType);
    if (it != CARTRIDGE_TYPES.end()) {
        return it->second;
    }
    return "Unknown Cartridge Type";
}

uint32_t Cartridge::getROMSize() const {
    auto it = ROM_SIZES.find(header.romSize);
    if (it != ROM_SIZES.end()) {
        return it->second;
    } else {
        return rom.size(); // Fallback to actual ROM size
    }
}

uint32_t Cartridge::getRAMSize() const {
    auto it = RAM_SIZES.find(header.ramSize);
    if (it != RAM_SIZES.end()) {
        return it->second;
    }
    return 0; // No RAM
}

Cartridge::Cartridge(const std::string& romPath) {
    loadFromFile(romPath);
}

Cartridge::~Cartridge() {
    // Save battery-backed RAM on destruction if needed
    if (hasBattery()) {
        saveRAM();
    }
}

bool Cartridge::loadFromFile(const std::string& romPath) {
    // Store ROM path for save files
    rom_path = romPath;
    
    // Open file
    std::ifstream file(romPath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        std::cerr << "Failed to open: " << romPath << std::endl;
        return false;
    }

    std::cout << "Opened: " << romPath << std::endl;

    // Get file size
    auto fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    // Read the entire ROM file into the rom vector
    rom.resize(fileSize);
    file.read(reinterpret_cast<char*>(rom.data()), fileSize);
    file.close();

    // Copy header data from ROM
    if (rom.size() < 0x150) {  // Check if ROM is big enough to contain header
        std::cerr << "ROM file too small to contain header" << std::endl;
        return false;
    }
    std::memcpy(&header, &rom[0x100], sizeof(CartridgeHeader));

    // Ensure title is null-terminated
    header.title[15] = 0;

    // Initialize RAM based on cartridge type
    initializeRAM();
    
    // Create appropriate MBC based on cartridge type
    createMBC();
    
    // Load RAM from save file if battery-backed
    if (hasBattery()) {
        loadRAM();
    }

    // Print cartridge information
    std::cout << "Cartridge Loaded:\n";
    std::cout << "\tTitle    : " << header.title << "\n";
    std::cout << "\tType     : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.cartridgeType) 
              << " (" << getCartridgeTypeName() << ")\n";
    std::cout << "\tROM Size : " << std::dec << (getROMSize() / 1024) << " KB\n";
    
    std::cout << "\tRAM Size : " << std::dec << (getRAMSize() / 1024) << " KB\n";
    
    std::cout << "\tLIC Code : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.oldLicenseCode) 
              << " (" << getPublisherName() << ")\n";
    std::cout << "\tROM Vers : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.versionNumber) << "\n";

    // Validate checksum
    validateCheckSum();

    return true;
}

void Cartridge::initializeRAM() {
    uint32_t ram_size = getRAMSize();
    
    // Special case for MBC2, which has 512x4 bits of RAM
    if (header.cartridgeType == 0x05 || header.cartridgeType == 0x06) {
        ram_size = 512;
    }
    
    // Resize RAM vector accordingly
    if (ram_size > 0) {
        ram.resize(ram_size, 0xFF);
    } else {
        ram.clear();
    }
}

void Cartridge::createMBC() {
    bool has_battery = false;
    
    // Check if cartridge has battery-backed RAM
    switch (header.cartridgeType) {
        case 0x03: // MBC1+RAM+BATTERY
        case 0x06: // MBC2+BATTERY
        case 0x09: // ROM+RAM+BATTERY
        case 0x0D: // MMM01+RAM+BATTERY
        case 0x0F: // MBC3+TIMER+BATTERY
        case 0x10: // MBC3+TIMER+RAM+BATTERY
        case 0x13: // MBC3+RAM+BATTERY
        case 0x1B: // MBC5+RAM+BATTERY
        case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
        case 0x22: // MBC7+SENSOR+RUMBLE+RAM+BATTERY
        case 0xFD: // BANDAI TAMA5
        case 0xFF: // HuC1+RAM+BATTERY
            has_battery = true;
            break;
    }
    
    // Create appropriate MBC based on cartridge type
    switch (header.cartridgeType) {
        case 0x00: // ROM ONLY
            mbc = std::make_unique<ROMOnly>(rom, ram);
            break;
            
        case 0x01: // MBC1
        case 0x02: // MBC1+RAM
        case 0x03: // MBC1+RAM+BATTERY
            mbc = std::make_unique<MBC1>(rom, ram, has_battery, false);
            break;
            
        case 0x05: // MBC2
        case 0x06: // MBC2+BATTERY
            mbc = std::make_unique<MBC2>(rom, ram, has_battery);
            break;
            
        case 0x0F: // MBC3+TIMER+BATTERY
        case 0x10: // MBC3+TIMER+RAM+BATTERY
            mbc = std::make_unique<MBC3>(rom, ram, has_battery, true);
            break;
            
        case 0x11: // MBC3
        case 0x12: // MBC3+RAM
        case 0x13: // MBC3+RAM+BATTERY
            mbc = std::make_unique<MBC3>(rom, ram, has_battery, false);
            break;
            
        case 0x19: // MBC5
        case 0x1A: // MBC5+RAM
        case 0x1B: // MBC5+RAM+BATTERY
            mbc = std::make_unique<MBC5>(rom, ram, has_battery, false);
            break;
            
        case 0x1C: // MBC5+RUMBLE
        case 0x1D: // MBC5+RUMBLE+RAM
        case 0x1E: // MBC5+RUMBLE+RAM+BATTERY
            mbc = std::make_unique<MBC5>(rom, ram, has_battery, true);
            break;
            
        default:
            // For unsupported MBC types, fallback to ROM only
            std::cerr << "Unsupported MBC type: " << std::hex << (int)header.cartridgeType << std::endl;
            mbc = std::make_unique<ROMOnly>(rom, ram);
            break;
    }
}

uint8_t Cartridge::read(uint16_t addr) const {
    return mbc->read(addr);
}

void Cartridge::write(uint16_t addr, uint8_t value) {
    mbc->write(addr, value);
}

bool Cartridge::saveRAM() const {
    if (!hasBattery() || ram.empty()) {
        return false;
    }
    
    // Generate save file path by replacing the extension with .sav
    std::string save_path = rom_path;
    size_t dot_pos = save_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        save_path = save_path.substr(0, dot_pos);
    }
    save_path += ".sav";
    
    return mbc->saveRAM(save_path);
}

bool Cartridge::loadRAM() {
    if (!hasBattery() || ram.empty()) {
        return false;
    }
    
    // Generate save file path by replacing the extension with .sav
    std::string save_path = rom_path;
    size_t dot_pos = save_path.find_last_of('.');
    if (dot_pos != std::string::npos) {
        save_path = save_path.substr(0, dot_pos);
    }
    save_path += ".sav";
    
    return mbc->loadRAM(save_path);
}

bool Cartridge::hasBattery() const {
    return mbc->hasBattery();
}

void Cartridge::validateCheckSum() const {
    // Calculate header checksum
    uint8_t checksum = 0;
    for (uint16_t i = 0x134; i <= 0x14C; i++) {
        checksum = checksum - rom[i] - 1;
    }
    
    // Compare with the value in the header
    bool header_checksum_valid = (checksum == header.headerChecksum);
    
    std::cout << "\tHeader Checksum : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.headerChecksum) 
              << " (" << (header_checksum_valid ? "VALID" : "INVALID") << ")\n";
    
    // Calculate global checksum (just for information, not validated by the Game Boy)
    uint16_t global_sum = 0;
    for (size_t i = 0; i < rom.size(); i++) {
        // Skip the global checksum bytes themselves
        if (i != 0x14E && i != 0x14F) {
            global_sum += rom[i];
        }
    }
    
    std::cout << "\tGlobal Checksum : " << std::hex << std::uppercase << std::setw(4) 
              << std::setfill('0') << global_sum
              << " (Expected: " << std::setw(4) << std::setfill('0') << header.globalChecksum << ")\n";
}






