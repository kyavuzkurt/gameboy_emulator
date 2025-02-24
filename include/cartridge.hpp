#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>


struct CartridgeHeader {
    uint8_t entryPoint[4];          // 0x100-0x103 
    uint8_t nintendoLogo[48];       // 0x103-0x133
    char title[16];                 // 0x134-0x143
    uint16_t newLicenseCode;        // 0x144-0x145
    uint8_t sgbFlag;                // 0x146
    uint8_t cartridgeType;          // 0x147
    uint8_t romSize;                // 0x148
    uint8_t ramSize;                // 0x149
    uint8_t destinationCode;        // 0x14A
    uint8_t oldLicenseCode;         // 0x14B
    uint8_t versionNumber;          // 0x14C
    uint8_t headerChecksum;         // 0x14D
    uint8_t globalChecksum;         // 0x14E-0x14F
};

class Cartridge {
    public:
        explicit Cartridge(const std::string& romPath);
        bool loadFromFile(const std::string& romPath);

        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);

        const CartridgeHeader& getHeader() const { return header; }
        std::string getPublisherName() const;
        std::string getCartridgeTypeName() const;

        uint32_t getROMSize() const;

        explicit Cartridge(const std::vector<uint8_t>& rom_data) : rom(rom_data) {}
    private:
        CartridgeHeader header;
        std::vector<uint8_t> rom;
        std::vector<uint8_t> ram;
        uint8_t currentRomBank = 1;
        uint8_t currentRamBank = 0;
        bool ramEnabled = false;

        void validateCheckSum() const;
};

