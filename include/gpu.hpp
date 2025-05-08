#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <iostream>
#include <vector>
#include <deque>

// Forward declaration of MemoryBus to avoid circular dependency
class MemoryBus;

// GameBoy screen dimensions
constexpr int SCREEN_WIDTH = 160;
constexpr int SCREEN_HEIGHT = 144;

// GPU timing constants (in CPU cycles)
constexpr int CYCLES_OAM = 80;        // Mode 2: OAM scan (80 cycles)
constexpr int CYCLES_TRANSFER = 172;  // Mode 3: Pixel transfer (172 cycles)
constexpr int CYCLES_HBLANK = 204;    // Mode 0: H-Blank (204 cycles)
constexpr int CYCLES_SCANLINE = 456;  // Complete scanline (456 cycles)
constexpr int VBLANK_LINES = 10;      // Number of scanlines in VBlank

// LCD Controller Modes
enum class LCDMode : uint8_t {
    HBLANK = 0,   // H-Blank period (CPU can access VRAM and OAM)
    VBLANK = 1,   // V-Blank period (CPU can access VRAM and OAM)
    OAM = 2,      // OAM scan period (CPU can access VRAM but not OAM)
    TRANSFER = 3  // Pixel transfer (CPU cannot access VRAM or OAM)
};

// Background/Window pixel information
struct BGPixelInfo {
    uint8_t colorIndex;  // Color index (0-3)
    bool bgPriority;     // BG-to-OAM priority (from attribute for CGB, unused for DMG)
    
    BGPixelInfo() : colorIndex(0), bgPriority(false) {}
    BGPixelInfo(uint8_t idx, bool pri) : colorIndex(idx), bgPriority(pri) {}
};

// Sprite pixel information
struct SpritePixelInfo {
    uint8_t colorIndex;   // Color index (0-3, 0 is transparent)
    uint8_t paletteNum;   // Palette number (0-1 for DMG, 0-7 for CGB)
    bool priority;        // Sprite priority (true = behind background)
    uint8_t spriteIdx;    // Original sprite index for prioritization
    
    SpritePixelInfo() : colorIndex(0), paletteNum(0), priority(false), spriteIdx(0) {}
    SpritePixelInfo(uint8_t idx, uint8_t pal, bool pri, uint8_t sprite) 
        : colorIndex(idx), paletteNum(pal), priority(pri), spriteIdx(sprite) {}
};

class GPU {
public:
    // Constructor
    explicit GPU(MemoryBus& memory);
    
    // Reset GPU state
    void reset();
    
    // Read from GPU memory-mapped registers and VRAM
    uint8_t read(uint16_t address) const;
    
    // Write to GPU memory-mapped registers and VRAM
    void write(uint16_t address, uint8_t value);
    
    // GPU tick function - updates GPU state based on elapsed CPU cycles
    void tick(uint64_t cycles);
    
    // Allow MemoryBus to query current LCD mode for VRAM access control
    LCDMode getCurrentMode() const { return current_mode; }
    
    // Set callbacks for interrupts
    void setVBlankInterruptCallback(std::function<void()> callback) {
        vblank_callback = callback;
    }
    void setLCDStatInterruptCallback(std::function<void()> callback) {
        lcd_stat_callback = callback;
    }
    
    // Get screen buffer for rendering
    const std::vector<uint32_t>& getScreenBuffer() const { return screen_buffer; }
    
    // Debug function to dump VRAM contents to a file
    void dumpVRAM(const std::string& filename);
    
    // Debug function to force VRAM check
    void forceVRAMCheck();
    
    // Debug function to dump detailed VRAM contents for analysis
    void dumpVRAMDebug();

private:
    // Reference to memory bus
    MemoryBus& memory;
    
    // Internal VRAM (8KB)
    std::array<uint8_t, 0x2000> vram;
    
    // GPU registers (0xFF40-0xFF4B)
    std::array<uint8_t, 12> registers;
    
    // Screen buffer
    std::vector<uint32_t> screen_buffer;
    
    // Current LCD mode
    LCDMode current_mode;
    
    // Cycle counter for current mode
    uint16_t mode_cycles;
    
    // Current scanline being processed
    int line;
    
    // Frame counter
    int frame_counter;
    
    // Flag for using debug pattern when no valid VRAM data is detected
    bool using_debug_pattern;
    
    // Cycles since last debug output
    uint64_t cycles_since_last_debug;
    
    // Interrupt callbacks
    std::function<void()> vblank_callback;
    std::function<void()> lcd_stat_callback;
    
    // GPU register addresses
    static constexpr uint16_t LCDC_REG = 0xFF40;  // LCD Control Register
    static constexpr uint16_t STAT_REG = 0xFF41;  // LCD Status Register
    static constexpr uint16_t SCY_REG = 0xFF42;   // Scroll Y Register
    static constexpr uint16_t SCX_REG = 0xFF43;   // Scroll X Register
    static constexpr uint16_t LY_REG = 0xFF44;    // LCD Y-Coordinate (current scanline)
    static constexpr uint16_t LYC_REG = 0xFF45;   // LY Compare Register
    static constexpr uint16_t BGP_REG = 0xFF47;   // Background Palette Register
    static constexpr uint16_t OBP0_REG = 0xFF48;  // Object Palette 0 Register
    static constexpr uint16_t OBP1_REG = 0xFF49;  // Object Palette 1 Register
    static constexpr uint16_t WY_REG = 0xFF4A;    // Window Y Position Register
    static constexpr uint16_t WX_REG = 0xFF4B;    // Window X Position Register
    
    // Pixel FIFO structures
    std::deque<BGPixelInfo> bg_fifo;
    std::deque<SpritePixelInfo> sprite_fifo;
    
    // Pixel FIFO state
    int fifo_x = 0;        // X position being processed by the FIFO
    int pixel_x = 0;       // Current pixel position being drawn to the screen
    bool window_active = false;  // Whether the window is currently being drawn
    uint8_t window_line = 0;    // Internal line counter for the window
    
    // Sprite data for the current scanline
    struct OAMEntry {
        uint8_t y;
        uint8_t x;
        uint8_t tile_idx;
        uint8_t attrs;
        uint8_t oam_idx;  // Original index in OAM for priority
        
        bool operator<(const OAMEntry& other) const {
            return x < other.x; // Sort by X position
        }
    };
    std::vector<OAMEntry> visible_sprites;  // Sprites visible on current scanline
    
    // Tile fetcher state
    enum class FetcherState { TILE, DATA_LOW, DATA_HIGH, PUSH };
    FetcherState fetcher_state = FetcherState::TILE;
    int fetcher_x = 0;     // X position being fetched
    uint8_t tile_idx = 0;  // Index of the tile being fetched
    uint8_t tile_data_low = 0; // Low byte of tile data
    uint8_t tile_data_high = 0; // High byte of tile data
    int fetcher_cycles = 0; // Cycle counter for the fetcher
    
    // Pixel FIFO methods
    void resetPixelFIFO();
    void fetchTileData();
    void pushPixelsToFIFO();
    void mixPixels();
    void drawPixel();
    
    // Old render functions - deprecated but kept for reference
    void renderScanline();
    void renderBackground(int scanline);
    void renderWindow(int scanline);
    void renderSprites(int scanline);
    
    // New pixel FIFO render methods
    void processScanline();
    void scanOAM();
    void fetchBackgroundTile();
    void fetchWindowTile();
    void fetchSpriteTile(const OAMEntry& sprite);
    void startPixelTransfer();
    void finalizeCurrentLine();
    
    // Check LY=LYC interrupt
    void checkLYC();
    
    // Get color from palette
    uint8_t getColorFromPalette(uint8_t palette, uint8_t colorIdx);
    
    // Check if VRAM contains valid data
    void checkVRAMData();
    
    // Convert Game Boy color to RGB32
    uint32_t getRGBColor(uint8_t gbColor);
    
    // Draw a test pattern directly to the screen buffer
    void drawTestPattern();
    
    // Helper function for drawing digits in the test pattern
    bool digitPattern(int digit, int x, int y);
    
    // Debug function to dump tilemap information
    void dumpTilemapDebug();
    
    // Calculate the duration of mode 3 for a given scanline
    uint16_t calculateMode3Duration(uint8_t scanline);
    
    // Helper functions
    bool isLCDEnabled() const;
    uint16_t getBackgroundTileMap() const;
    uint16_t getWindowTileMap() const;
    uint16_t getTileDataAddress() const;
    bool areBGAndWindowEnabled() const;
    bool areSpritesEnabled() const;
    bool isWindowEnabled() const;
    uint8_t getSpriteHeight() const;
};
