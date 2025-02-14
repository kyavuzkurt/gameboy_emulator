#include "cartridge.hpp"
#include <fstream>
#include <stdexcept>
#include <iostream>
#include <unordered_map>
#include <iomanip>
#include <cstring>

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

std::string Cartridge::getPublisherName() const {
    auto it = NEW_LICENSE_CODES.find(header.newLicenseCode);
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

std::uint32_t Cartridge::getROMSize() const {
    // Formula: 32 KB * (1 << value)
    return 32768u * (1u << header.romSize);
}

Cartridge::Cartridge(const std::string& romPath) {
    if (!loadFromFile(romPath)) {
        throw std::runtime_error("Failed to load ROM");
    }
}

bool Cartridge::loadFromFile(const std::string& romPath) {
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

    // Print cartridge information
    std::cout << "Cartridge Loaded:\n";
    std::cout << "\tTitle    : " << header.title << "\n";
    std::cout << "\tType     : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.cartridgeType) 
              << " (" << getCartridgeTypeName() << ")\n";
    std::cout << "\tROM Size : " << std::dec << (getROMSize() / 1024) << " KB\n";
    std::cout << "\tRAM Size : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.ramSize) << "\n";
    std::cout << "\tLIC Code : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.newLicenseCode) 
              << " (" << getPublisherName() << ")\n";
    std::cout << "\tROM Vers : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.versionNumber) << "\n";

    // Calculate checksum
    uint16_t x = 0;
    for (uint16_t i = 0x0134; i <= 0x014C; i++) {
        x = x - rom[i] - 1;
    }

    std::cout << "\tChecksum : " << std::hex << std::uppercase << std::setw(2) 
              << std::setfill('0') << static_cast<int>(header.headerChecksum) 
              << " (" << ((x & 0xFF) ? "PASSED" : "FAILED") << ")\n";

    return true;
}






