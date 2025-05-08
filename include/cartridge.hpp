#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <fstream>

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
    uint16_t globalChecksum;        // 0x14E-0x14F
};

// Base MBC class
class MBC {
public:
    virtual ~MBC() = default;
    
    virtual uint8_t read(uint16_t addr) const = 0;
    virtual void write(uint16_t addr, uint8_t value) = 0;
    
    // Save RAM to file if battery-backed
    virtual bool saveRAM(const std::string& save_path) const { return false; }
    
    // Load RAM from file if battery-backed
    virtual bool loadRAM(const std::string& save_path) { return false; }
    
    // Check if this MBC has battery-backed RAM
    virtual bool hasBattery() const { return false; }
};

// No MBC (ROM only) implementation
class ROMOnly : public MBC {
public:
    ROMOnly(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram);
    
    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;
    
private:
    const std::vector<uint8_t>& rom;
    std::vector<uint8_t>& ram;
    bool ram_enabled = false;
};

// MBC1 implementation (up to 2MB ROM, 32KB RAM)
class MBC1 : public MBC {
public:
    MBC1(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery, bool is_multicart = false);
    
    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;
    
    bool saveRAM(const std::string& save_path) const override;
    bool loadRAM(const std::string& save_path) override;
    bool hasBattery() const override { return battery; }
    
private:
    const std::vector<uint8_t>& rom;
    std::vector<uint8_t>& ram;
    bool ram_enabled = false;
    uint8_t rom_bank = 1;          // 5-bit register, 0 is treated as 1
    uint8_t ram_bank = 0;          // 2-bit register, for RAM banking or upper ROM bits
    bool mode_select = false;      // 0 = Simple mode, 1 = Advanced mode
    bool battery = false;          // Has battery-backed RAM
    bool multicart = false;        // Is MBC1M multicart
    
    uint32_t getRomBankStart() const;
    uint32_t getRamBankStart() const;
};

// MBC2 implementation (up to 256KB ROM, 512x4 bits RAM)
class MBC2 : public MBC {
public:
    MBC2(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery);
    
    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;
    
    bool saveRAM(const std::string& save_path) const override;
    bool loadRAM(const std::string& save_path) override;
    bool hasBattery() const override { return battery; }
    
private:
    const std::vector<uint8_t>& rom;
    std::vector<uint8_t>& ram;     // 512x4 bits RAM
    bool ram_enabled = false;
    uint8_t rom_bank = 1;          // 4-bit register, 0 is treated as 1
    bool battery = false;          // Has battery-backed RAM
};

// MBC3 implementation (up to 2MB ROM, 32KB RAM, RTC)
class MBC3 : public MBC {
public:
    MBC3(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery, bool has_rtc);
    
    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;
    
    bool saveRAM(const std::string& save_path) const override;
    bool loadRAM(const std::string& save_path) override;
    bool hasBattery() const override { return battery; }
    
private:
    const std::vector<uint8_t>& rom;
    std::vector<uint8_t>& ram;
    bool ram_enabled = false;
    uint8_t rom_bank = 1;          // 7-bit register, 0 is treated as 1
    uint8_t ram_bank = 0;          // RAM bank or RTC register select
    bool battery = false;          // Has battery-backed RAM
    bool rtc = false;              // Has Real-Time Clock
    
    // RTC registers
    uint8_t rtc_s = 0;             // Seconds (0-59)
    uint8_t rtc_m = 0;             // Minutes (0-59)
    uint8_t rtc_h = 0;             // Hours (0-23)
    uint8_t rtc_dl = 0;            // Lower 8 bits of day counter
    uint8_t rtc_dh = 0;            // Upper 1 bit of day counter, halt flag, day counter carry flag
    
    bool rtc_latch = false;        // RTC latch state
    
    // Latched RTC registers (for reading)
    uint8_t latch_rtc_s = 0;
    uint8_t latch_rtc_m = 0;
    uint8_t latch_rtc_h = 0;
    uint8_t latch_rtc_dl = 0;
    uint8_t latch_rtc_dh = 0;
    
    void latchRTC();
    void updateRTC();
    void saveRTC(const std::string& save_path) const;
    bool loadRTC(const std::string& save_path);
};

// MBC5 implementation (up to 8MB ROM, 128KB RAM)
class MBC5 : public MBC {
public:
    MBC5(const std::vector<uint8_t>& rom_data, std::vector<uint8_t>& ram, bool has_battery, bool has_rumble);
    
    uint8_t read(uint16_t addr) const override;
    void write(uint16_t addr, uint8_t value) override;
    
    bool saveRAM(const std::string& save_path) const override;
    bool loadRAM(const std::string& save_path) override;
    bool hasBattery() const override { return battery; }
    
private:
    const std::vector<uint8_t>& rom;
    std::vector<uint8_t>& ram;
    bool ram_enabled = false;
    uint16_t rom_bank = 1;         // 9-bit register (0-511)
    uint8_t ram_bank = 0;          // 4-bit register (0-15)
    bool battery = false;          // Has battery-backed RAM
    bool rumble = false;           // Has rumble feature
};

class Cartridge {
    public:
        explicit Cartridge(const std::string& romPath);
        ~Cartridge();
        
        bool loadFromFile(const std::string& romPath);

        uint8_t read(uint16_t addr) const;
        void write(uint16_t addr, uint8_t value);

        const CartridgeHeader& getHeader() const { return header; }
        std::string getPublisherName() const;
        std::string getCartridgeTypeName() const;

        uint32_t getROMSize() const;
        uint32_t getRAMSize() const;

        // Get the title of the ROM
        std::string getTitle() const {
            return std::string(reinterpret_cast<const char*>(header.title));
        }
        
        // Save battery-backed RAM to file
        bool saveRAM() const;
        
        // Load battery-backed RAM from file
        bool loadRAM();
        
        // Check if cartridge has battery
        bool hasBattery() const;
        
    private:
        CartridgeHeader header;
        std::vector<uint8_t> rom;
        std::vector<uint8_t> ram;
        std::unique_ptr<MBC> mbc;
        std::string rom_path;  // Keep the ROM path for save files
        
        void initializeRAM();
        void createMBC();
        
        void validateCheckSum() const;
};

