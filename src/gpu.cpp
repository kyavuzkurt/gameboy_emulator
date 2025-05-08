#include "gpu.hpp"
#include "memory.hpp"
#include <fstream>
#include <iostream>
#include <iomanip>  // Make sure this is included for I/O manipulators
#include <algorithm>
#include <sstream>

// Explicitly bring the needed I/O manipulators into scope
using std::setw;
using std::setfill;
using std::setprecision;

GPU::GPU(MemoryBus& memory) : memory(memory), current_mode(LCDMode::HBLANK), mode_cycles(0), line(0), frame_counter(0), using_debug_pattern(true), cycles_since_last_debug(0) {
    screen_buffer.resize(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFFFFFFFF); // Initialize to white
    
    // Initialize VRAM to zeros
    vram.fill(0);
    
    // Force a debug pattern in VRAM right from the start
    // Create a simple checkerboard pattern in the first few tiles
    for (int tile = 0; tile < 256; tile++) {
        for (int y = 0; y < 8; y++) {
            uint8_t pattern = (y % 2) ? 0xAA : 0x55;
            // Each row is defined by 2 bytes (low/high bits)
            vram[tile * 16 + y * 2] = pattern;
            vram[tile * 16 + y * 2 + 1] = pattern;
        }
    }
    
    // Fill tilemap with non-zero tile indices
    for (int i = 0; i < 32 * 32; i++) {
        vram[0x1800 + i] = 1; // Use tile #1 for the background map
        vram[0x1C00 + i] = 2; // Use tile #2 for the window map
    }
    
    std::cout << "GPU initialized with test pattern" << std::endl;
}

void GPU::tick(uint64_t cycles) {
    // Early return if LCD is disabled
    if (!isLCDEnabled()) {
        // When LCD is disabled, LY is set to 0 and the mode is set to VBLANK
        memory.write(LY_REG, 0);
        uint8_t stat = memory.read(STAT_REG);
        stat &= 0xFC; // Clear mode bits
        stat |= static_cast<uint8_t>(LCDMode::VBLANK);
        memory.write(STAT_REG, stat);
        return;
    }
    
    // Accumulate cycles
    mode_cycles += cycles;
    cycles_since_last_debug += cycles;
    
    // Update LCD status based on current mode and line
    // The PPU state machine has 4 modes:
    // Mode 2 (OAM Scan): 80 cycles
    // Mode 3 (Pixel Transfer): Variable (172-289) cycles
    // Mode 0 (HBlank): Remaining cycles to 456 total
    // Mode 1 (VBlank): 10 lines of 456 cycles each
    
    // Debug: Every 1,000,000 cycles, log the current GPU state
    if (cycles_since_last_debug >= 1000000) {
        cycles_since_last_debug = 0;
        std::cout << "GPU Mode: " << static_cast<int>(current_mode) 
                  << ", Line: " << static_cast<int>(memory.read(LY_REG))
                  << ", Mode cycles: " << mode_cycles 
                  << ", LCDC: 0x" << std::hex << static_cast<int>(memory.read(LCDC_REG)) 
                  << ", STAT: 0x" << static_cast<int>(memory.read(STAT_REG)) << std::dec << std::endl;
    }
    
    // Get the current line
    uint8_t current_line = memory.read(LY_REG);
    
    // Process the current state
    switch (current_mode) {
        case LCDMode::OAM: {
            // Mode 2 - OAM Scan (80 cycles)
            // During this time, OAM is inaccessible
            if (mode_cycles >= CYCLES_OAM) {
                // Prepare the scanline data for rendering
                scanOAM();
                
                // Move to pixel transfer mode
                current_mode = LCDMode::TRANSFER;
                startPixelTransfer();
                
                // Update STAT register
                uint8_t stat = memory.read(STAT_REG);
                stat &= 0xFC; // Clear mode bits
                stat |= static_cast<uint8_t>(LCDMode::TRANSFER);
                memory.write(STAT_REG, stat);
                
                // Reset the cycle counter for mode 3
                mode_cycles = mode_cycles - CYCLES_OAM;
            }
            break;
        }
        
        case LCDMode::TRANSFER: {
            // Mode 3 - Pixel Transfer (variable duration)
            // Calculate the duration based on rendering properties
            uint16_t mode3_duration = calculateMode3Duration(current_line);
            
            // Process the current pixel fetching and FIFO operation
            processScanline();
            
            // Check if we've completed the transfer phase
            if (mode_cycles >= mode3_duration) {
                // Finalize the scanline
                finalizeCurrentLine();
                
                // Move to HBlank
                current_mode = LCDMode::HBLANK;
                
                // Update STAT register
                uint8_t stat = memory.read(STAT_REG);
                stat &= 0xFC; // Clear mode bits
                stat |= static_cast<uint8_t>(LCDMode::HBLANK);
                memory.write(STAT_REG, stat);
                
                // Check if HBlank interrupt is enabled
                if (stat & 0x08) {
                    if (lcd_stat_callback) {
                        lcd_stat_callback();
                    }
                }
                
                // Reset cycle counter for mode 0
                mode_cycles = mode_cycles - mode3_duration;
            }
            break;
        }
        
        case LCDMode::HBLANK: {
            // Mode 0 - HBlank
            uint16_t hblank_duration = CYCLES_SCANLINE - CYCLES_OAM - calculateMode3Duration(current_line);
            
            if (mode_cycles >= hblank_duration) {
                // Advance to the next line
                current_line = (current_line + 1) % 154;
                memory.write(LY_REG, current_line);
                
                // Check LYC=LY condition
                checkLYC();
                
                // Reset cycle counter
                mode_cycles = mode_cycles - hblank_duration;
                
                // Check if we've reached the VBlank period (line 144)
                if (current_line == 144) {
                    // Enter VBlank
                    current_mode = LCDMode::VBLANK;
                    
                    // Update STAT register
                    uint8_t stat = memory.read(STAT_REG);
                    stat &= 0xFC; // Clear mode bits
                    stat |= static_cast<uint8_t>(LCDMode::VBLANK);
                    memory.write(STAT_REG, stat);
                    
                    // Check if VBlank interrupt is enabled in STAT
                    if (stat & 0x10) {
                        if (lcd_stat_callback) {
                            lcd_stat_callback();
                        }
                    }
                    
                    // Trigger VBlank interrupt
                    if (vblank_callback) {
                        vblank_callback();
                    }
                    
                    // Increment frame counter
                    frame_counter++;
                    
                    // MODIFIED: Only draw test pattern during first 10 frames
                    if (frame_counter <= 10) {
                        drawTestPattern();
                        std::cout << "Frame " << frame_counter << ": Drawing test pattern" << std::endl;
                    } else if (frame_counter % 60 == 0) {
                        // Every 60 frames, dump tilemap for debugging
                        dumpTilemapDebug();
                    }
                    
                    // Only in VBlank: Check VRAM for valid data (debug purposes)
                    if (frame_counter % 30 == 0) {
                        checkVRAMData();
                    }
                } else {
                    // Start a new OAM scan
                    current_mode = LCDMode::OAM;
                    
                    // Update STAT register
                    uint8_t stat = memory.read(STAT_REG);
                    stat &= 0xFC; // Clear mode bits
                    stat |= static_cast<uint8_t>(LCDMode::OAM);
                    memory.write(STAT_REG, stat);
                    
                    // Check if OAM interrupt is enabled
                    if (stat & 0x20) {
                        if (lcd_stat_callback) {
                            lcd_stat_callback();
                        }
                    }
                }
            }
            break;
        }
        
        case LCDMode::VBLANK: {
            // Mode 1 - VBlank (10 lines of 456 cycles each)
            if (mode_cycles >= CYCLES_SCANLINE) {
                // Advance to the next line
                current_line = (current_line + 1) % 154;
                memory.write(LY_REG, current_line);
                
                // Check LYC=LY condition
                checkLYC();
                
                // Reset cycle counter
                mode_cycles = mode_cycles - CYCLES_SCANLINE;
                
                // Check if VBlank is finished
                if (current_line == 0) {
                    // Start a new frame with OAM scan
                    current_mode = LCDMode::OAM;
                    
                    // Reset window line counter
                    window_line = 0;
                    
                    // Update STAT register
                    uint8_t stat = memory.read(STAT_REG);
                    stat &= 0xFC; // Clear mode bits
                    stat |= static_cast<uint8_t>(LCDMode::OAM);
                    memory.write(STAT_REG, stat);
                    
                    // Check if OAM interrupt is enabled
                    if (stat & 0x20) {
                        if (lcd_stat_callback) {
                            lcd_stat_callback();
                        }
                    }
                }
            }
            break;
        }
    }
}

// PIXEL FIFO IMPLEMENTATION METHODS

void GPU::resetPixelFIFO() {
    // Clear both FIFOs
    bg_fifo.clear();
    sprite_fifo.clear();
    
    // Reset FIFO state
    fifo_x = 0;
    pixel_x = 0;
    fetcher_x = 0;
    fetcher_state = FetcherState::TILE;
    fetcher_cycles = 0;
    window_active = false;
}

void GPU::scanOAM() {
    // Check if sprites are enabled
    if (!areSpritesEnabled()) {
        visible_sprites.clear();
        return;
    }
    
    // Get the current scanline
    uint8_t current_line = memory.read(LY_REG);
    uint8_t sprite_height = getSpriteHeight();
    
    // Clear previous sprite data
    visible_sprites.clear();
    
    // Scan OAM for sprites that intersect with this scanline
    for (int i = 0; i < 40; i++) {
        uint16_t oam_addr = 0xFE00 + (i * 4);
        uint8_t y_pos = memory.read(oam_addr) - 16;  // Y position is offset by 16
        
        // Check if sprite is on this scanline
        if (current_line >= y_pos && current_line < y_pos + sprite_height) {
            // Sprite is visible on this scanline
            OAMEntry sprite;
            sprite.y = y_pos;
            sprite.x = memory.read(oam_addr + 1) - 8;  // X position is offset by 8
            sprite.tile_idx = memory.read(oam_addr + 2);
            sprite.attrs = memory.read(oam_addr + 3);
            sprite.oam_idx = i;  // Store original OAM index for priority
            
            // Add to visible sprites list
            visible_sprites.push_back(sprite);
            
            // Game Boy hardware limited to 10 sprites per scanline
            if (visible_sprites.size() >= 10) {
                break;
            }
        }
    }
    
    // Sort sprites by X-position for proper rendering order
    std::sort(visible_sprites.begin(), visible_sprites.end());
}

void GPU::startPixelTransfer() {
    // Initialize FIFO state for the current scanline
    resetPixelFIFO();
    
    // Reset window active flag if window is enabled on this scanline
    if (isWindowEnabled()) {
        uint8_t wy = memory.read(WY_REG);
        window_active = false;  // Will be set to true when X position is reached
    }
}

void GPU::processScanline() {
    // Process the fetcher and FIFO for the current pixel being rendered
    while (pixel_x < SCREEN_WIDTH && mode_cycles < calculateMode3Duration(memory.read(LY_REG))) {
        // Advance the tile fetcher state machine, which runs at 2MHz (half the CPU clock)
        if (++fetcher_cycles % 2 == 0) {
            fetchTileData();
        }
        
        // Check if we should switch to window tile fetching
        if (isWindowEnabled() && !window_active) {
            uint8_t wx = memory.read(WX_REG) - 7;  // WX is offset by 7
            uint8_t wy = memory.read(WY_REG);
            uint8_t current_line = memory.read(LY_REG);
            
            if (current_line >= wy && pixel_x >= wx) {
                // Switch to window tile fetching
                window_active = true;
                fetcher_x = 0;  // Reset fetcher to start of window
                fetcher_state = FetcherState::TILE;  // Restart fetcher
                bg_fifo.clear();  // Clear the BG FIFO to start fresh with window
            }
        }
        
        // If we have pixels in the FIFO, we can draw one
        if (!bg_fifo.empty()) {
            drawPixel();
        }
    }
}

void GPU::fetchTileData() {
    // Get the current line
    uint8_t current_line = memory.read(LY_REG);
    
    // State machine for tile fetching
    switch (fetcher_state) {
        case FetcherState::TILE: {
            // Get tile index
            uint16_t tile_map;
            uint8_t x_pos, y_pos;
            
            if (window_active) {
                // Calculate window tile map and position
                tile_map = getWindowTileMap();
                x_pos = fetcher_x;
                y_pos = window_line;
            } else {
                // Calculate background tile map and position
                tile_map = getBackgroundTileMap();
                x_pos = (memory.read(SCX_REG) / 8 + fetcher_x) & 0x1F;  // Wrap around at 32 tiles
                y_pos = (memory.read(SCY_REG) + current_line) & 0xFF;
            }
            
            // Get the tile index from the tile map
            uint16_t tile_map_addr = tile_map + (y_pos / 8) * 32 + x_pos;
            tile_idx = memory.read(tile_map_addr);
            
            // Debug output - dramatically reduce frequency
            static int tile_fetch_count = 0;
            if (++tile_fetch_count % 500000 == 0) {
                std::cout << "TILE FETCH: map addr=0x" << std::hex << tile_map_addr
                          << ", tile_idx=" << static_cast<int>(tile_idx)
                          << ", at x=" << std::dec << static_cast<int>(x_pos)
                          << ", y=" << static_cast<int>(y_pos / 8)
                          << ", fetcher_x=" << fetcher_x
                          << ", line=" << static_cast<int>(current_line)
                          << ", window=" << (window_active ? "YES" : "NO") << std::endl;
            }
            
            // Advance to next state
            fetcher_state = FetcherState::DATA_LOW;
            break;
        }
        
        case FetcherState::DATA_LOW: {
            // Get the data address for the tile
            uint16_t tile_data_addr = getTileDataAddress();
            uint8_t current_line = memory.read(LY_REG);
            uint8_t y_pos;
            
            if (window_active) {
                y_pos = window_line % 8;
            } else {
                y_pos = (memory.read(SCY_REG) + current_line) % 8;
            }
            
            // Calculate the exact address based on the tile index and addressing mode
            uint16_t tile_addr;
            if (tile_data_addr == 0x8000) {
                // Unsigned addressing mode
                tile_addr = tile_data_addr + tile_idx * 16;
            } else {
                // Signed addressing mode
                tile_addr = tile_data_addr + (static_cast<int8_t>(tile_idx) + 128) * 16;
            }
            
            // Read the low byte of the tile data for this row
            tile_data_low = memory.read(tile_addr + y_pos * 2);
            
            // Debug output - drastically reduce frequency
            static int data_low_count = 0;
            if (++data_low_count % 500000 == 0) {
                std::cout << "TILE DATA LOW: addr=0x" << std::hex << (tile_addr + y_pos * 2)
                          << ", data=0x" << static_cast<int>(tile_data_low)
                          << ", tile_idx=" << static_cast<int>(tile_idx)
                          << ", y_offset=" << std::dec << static_cast<int>(y_pos) << std::endl;
            }
            
            // Advance to next state
            fetcher_state = FetcherState::DATA_HIGH;
            break;
        }
        
        case FetcherState::DATA_HIGH: {
            // Get the data address for the tile
            uint16_t tile_data_addr = getTileDataAddress();
            uint8_t current_line = memory.read(LY_REG);
            uint8_t y_pos;
            
            if (window_active) {
                y_pos = window_line % 8;
    } else {
                y_pos = (memory.read(SCY_REG) + current_line) % 8;
            }
            
            // Calculate the exact address based on the tile index and addressing mode
            uint16_t tile_addr;
            if (tile_data_addr == 0x8000) {
                // Unsigned addressing mode
                tile_addr = tile_data_addr + tile_idx * 16;
            } else {
                // Signed addressing mode
                tile_addr = tile_data_addr + (static_cast<int8_t>(tile_idx) + 128) * 16;
            }
            
            // Read the high byte of the tile data for this row
            tile_data_high = memory.read(tile_addr + y_pos * 2 + 1);
            
            // Debug output - drastically reduce frequency 
            static int data_high_count = 0;
            if (++data_high_count % 500000 == 0) {
                std::cout << "TILE DATA HIGH: addr=0x" << std::hex << (tile_addr + y_pos * 2 + 1)
                          << ", data=0x" << static_cast<int>(tile_data_high)
                          << ", combined data pattern:";
                for (int bit = 7; bit >= 0; bit--) {
                    uint8_t color_low = (tile_data_low >> bit) & 0x01;
                    uint8_t color_high = (tile_data_high >> bit) & 0x01;
                    uint8_t color_idx = (color_high << 1) | color_low;
                    std::cout << " " << static_cast<int>(color_idx);
                }
                std::cout << std::dec << std::endl;
            }
            
            // Advance to next state
            fetcher_state = FetcherState::PUSH;
            break;
        }
        
        case FetcherState::PUSH: {
            // Only push pixels to the FIFO if it has room (less than 8 pixels)
            if (bg_fifo.size() <= 8) {
                // Debug output before pushing pixels - drastically reduce frequency
                static int push_count = 0;
                bool should_log = (++push_count % 500000 == 0);
                
                if (should_log) {
                    std::cout << "PUSHING PIXELS TO FIFO: low=0x" << std::hex 
                              << static_cast<int>(tile_data_low) << ", high=0x" 
                              << static_cast<int>(tile_data_high) << std::dec 
                              << ", pattern=";
                }
                
                // Push 8 pixels to the FIFO
                for (int bit = 7; bit >= 0; bit--) {
                    uint8_t color_low = (tile_data_low >> bit) & 0x01;
                    uint8_t color_high = (tile_data_high >> bit) & 0x01;
                    uint8_t color_idx = (color_high << 1) | color_low;
                    
                    // Log the pixel being pushed if it's time
                    if (should_log) {
                        std::cout << static_cast<int>(color_idx);
                        if (bit > 0) std::cout << " ";
                    }
                    
                    bg_fifo.emplace_back(color_idx, false);
                }
                
                if (should_log) {
                    std::cout << ", fifo size after push: " << bg_fifo.size() << std::endl;
                }
                
                // Move to the next tile
                fetcher_x++;
                fetcher_state = FetcherState::TILE;
            }
            break;
        }
    }
    
    // Check for sprites overlapping with the current pixels
    mixPixels();
}

void GPU::mixPixels() {
    // Skip if sprites are disabled or BG FIFO is empty
    if (!areSpritesEnabled() || bg_fifo.empty()) {
        return;
    }
    
    // Process sprites that overlap with the current pixel position
    for (const auto& sprite : visible_sprites) {
        // Check if the sprite is in range of the current pixel position
        int sprite_x = sprite.x;
        if (pixel_x + bg_fifo.size() > sprite_x && pixel_x <= sprite_x + 8) {
            // This sprite overlaps with the FIFO
            fetchSpriteTile(sprite);
        }
    }
}

void GPU::fetchSpriteTile(const OAMEntry& sprite) {
    // Get sprite attributes
    bool y_flip = (sprite.attrs & 0x40) != 0;
    bool x_flip = (sprite.attrs & 0x20) != 0;
    bool palette_num = (sprite.attrs & 0x10) != 0;
    bool priority = (sprite.attrs & 0x80) != 0;
    
    // Get current line and sprite height
    uint8_t current_line = memory.read(LY_REG);
    uint8_t sprite_height = getSpriteHeight();
    
    // Calculate which row of the sprite we're drawing
    uint8_t row = current_line - sprite.y;
    
    // Apply Y-flip if needed
    if (y_flip) {
        row = sprite_height - 1 - row;
    }
    
    // Get tile index and adjust for 8x16 sprites
    uint8_t tile = sprite.tile_idx;
    if (sprite_height == 16) {
        // For 8x16 sprites, bit 0 of the tile number is ignored
        tile &= 0xFE;
        
        // If we're in the second half of the sprite, use the next tile
        if (row >= 8) {
            tile++;
            row -= 8;
        }
    }
    
    // Calculate the tile data address (sprite tiles always use 0x8000 addressing)
    uint16_t tile_addr = 0x8000 + tile * 16 + row * 2;
    
    // Read the tile data for this row
    uint8_t data_low = memory.read(tile_addr);
    uint8_t data_high = memory.read(tile_addr + 1);
    
    // Process all 8 pixels of the sprite
    for (int x = 0; x < 8; x++) {
        // Apply X-flip if needed
        uint8_t bit = x_flip ? x : 7 - x;
        
        // Get color index
        uint8_t color_idx = ((data_low >> bit) & 0x01) | (((data_high >> bit) & 0x01) << 1);
        
        // Skip transparent pixels (color 0)
        if (color_idx == 0) {
            continue;
        }
        
        // Calculate the pixel position relative to the screen
        int screen_x = sprite.x + x;
        
        // Skip if pixel is offscreen or before the current FIFO position
        if (screen_x < 0 || screen_x >= SCREEN_WIDTH || screen_x < pixel_x) {
            continue;
        }
        
        // Add sprite pixel to the sprite FIFO at the correct position
        int fifo_index = screen_x - pixel_x;
        if (fifo_index < bg_fifo.size()) {
            // Replace existing sprite pixel only if this one has higher priority
            SpritePixelInfo pixel_info(color_idx, palette_num, priority, sprite.oam_idx);
            
            // Insert or update sprite pixel in the FIFO
            if (sprite_fifo.size() <= fifo_index) {
                // Expand the sprite FIFO if needed
                sprite_fifo.resize(fifo_index + 1);
            }
            sprite_fifo[fifo_index] = pixel_info;
        }
    }
}

void GPU::drawPixel() {
    // Get current line
    uint8_t current_line = memory.read(LY_REG);
    
    // Pop off the front pixel from the FIFO
    BGPixelInfo bg_pixel = {0, false};
    if (!bg_fifo.empty()) {
        bg_pixel = bg_fifo.front();
        bg_fifo.pop_front();
    }
    
    // Get sprite pixel (if any) for this location
    SpritePixelInfo sprite_pixel;
    if (!sprite_fifo.empty()) {
        sprite_pixel = sprite_fifo.front();
        sprite_fifo.pop_front();
    }
    
    // Debug: Log only every 500,000th pixel being drawn to verify the rendering pipeline
    static int pixel_counter = 0;
    if (++pixel_counter % 500000 == 0) {
        std::cout << "Drawing pixel #" << pixel_counter << " at position " 
                  << pixel_x << ", " << current_line 
                  << " with BG color: " << static_cast<int>(bg_pixel.colorIndex)
                  << ", sprite color: " << static_cast<int>(sprite_pixel.colorIndex) << std::endl;
    }
    
    // Determine the final color
    uint8_t final_color_idx = 0;
    
    // Check if background should be visible
    bool bg_enabled = areBGAndWindowEnabled();
    
    // Log BGP register value much less frequently
    static int bgp_debug_count = 0;
    if (bgp_debug_count++ % 500000 == 0) {
        uint8_t bg_palette = memory.read(BGP_REG);
        std::cout << "BGP register value: 0x" << std::hex << static_cast<int>(bg_palette)
                  << std::dec << " (BG Enabled: " << (bg_enabled ? "YES" : "NO") << ")" << std::endl;
    }
    
    if (bg_enabled) {
        // Background is enabled
        if (sprite_pixel.colorIndex != 0) {
            // Sprite pixel is not transparent
            // Check sprite priority
            if (sprite_pixel.priority && bg_pixel.colorIndex != 0) {
                // BG has priority over sprite (when sprite attr bit 7 is set and BG is not color 0)
                final_color_idx = bg_pixel.colorIndex;
                
                // Get the background palette
                uint8_t bg_palette = memory.read(BGP_REG);
                
                // Get the color from the palette
                final_color_idx = getColorFromPalette(bg_palette, final_color_idx);
            } else {
                // Sprite has priority
                final_color_idx = sprite_pixel.colorIndex;
                
                // Get the sprite palette
                uint8_t sprite_palette = sprite_pixel.paletteNum ? memory.read(OBP1_REG) : memory.read(OBP0_REG);
                
                // Get the color from the palette
                final_color_idx = getColorFromPalette(sprite_palette, final_color_idx);
            }
        } else {
            // Sprite pixel is transparent, use background
            final_color_idx = bg_pixel.colorIndex;
            
            // Get the background palette
            uint8_t bg_palette = memory.read(BGP_REG);
            
            // Get the color from the palette
            final_color_idx = getColorFromPalette(bg_palette, final_color_idx);
        }
    } else if (sprite_pixel.colorIndex != 0) {
        // Background is disabled but sprite is visible
        final_color_idx = sprite_pixel.colorIndex;
        
        // Get the sprite palette
        uint8_t sprite_palette = sprite_pixel.paletteNum ? memory.read(OBP1_REG) : memory.read(OBP0_REG);
        
        // Get the color from the palette
        final_color_idx = getColorFromPalette(sprite_palette, final_color_idx);
    } else {
        // Both background and sprite are invisible or disabled
        // Use color 0 (now red in our debug palette)
        final_color_idx = 0;
    }
    
    // Convert to final color and set pixel
    // Ensure we never go out of bounds
    if (current_line < SCREEN_HEIGHT && pixel_x < SCREEN_WIDTH) {
        screen_buffer[current_line * SCREEN_WIDTH + pixel_x] = getRGBColor(final_color_idx);
    }
    
    // ALWAYS increment pixel position - this was a potential bug in the original code
    pixel_x++;
}

void GPU::finalizeCurrentLine() {
    // Update window line counter if window was active on this scanline
    if (window_active) {
        window_line++;
    }
}

// HELPER METHODS

bool GPU::isLCDEnabled() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x80) != 0;
}

uint16_t GPU::getBackgroundTileMap() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x08) ? 0x9C00 : 0x9800;
}

uint16_t GPU::getWindowTileMap() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x40) ? 0x9C00 : 0x9800;
}

uint16_t GPU::getTileDataAddress() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x10) ? 0x8000 : 0x8800;
}

bool GPU::areBGAndWindowEnabled() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x01) != 0;
}

bool GPU::areSpritesEnabled() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x02) != 0;
}

bool GPU::isWindowEnabled() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x20) != 0;
}

uint8_t GPU::getSpriteHeight() const {
    uint8_t lcdc = memory.read(LCDC_REG);
    return (lcdc & 0x04) ? 16 : 8;
}

// THE SECTIONS BELOW ARE KEPT FROM THE ORIGINAL IMPLEMENTATION

void GPU::renderScanline() {
    // Placeholder implementation - will be expanded later
    uint8_t ly = memory.read(0xFF44);
    uint8_t lcdc = memory.read(0xFF40);
    
    // Only render if LCD is enabled (bit 7 of LCDC)
    if (lcdc & 0x80) {
        // Render background, window, and sprites in that order
        renderBackground(ly);
        renderWindow(ly);
        renderSprites(ly);
    }
    
    // Debug: Check if screen buffer contains any non-white pixels
    bool has_content = false;
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        if (screen_buffer[ly * SCREEN_WIDTH + x] != 0xFFFFFFFF) {
            has_content = true;
            break;
        }
    }
    
    // If this is the first scanline with content, log it
    static bool logged_content = false;
    if (has_content && !logged_content) {
        std::cout << "First non-white pixel detected on scanline " << (int)ly << std::endl;
        logged_content = true;
    }
    
    // Every 60 frames, check the composition of the screen buffer
    if (frame_counter % 60 == 0 && memory.read(LY_REG) == 80) {
        // Count different colors in screen buffer to see what's being rendered
        int white_pixels = 0, non_white_pixels = 0;
        
        for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
            if (screen_buffer[i] == 0xFFFFFFFF) {
                white_pixels++;
            } else {
                non_white_pixels++;
            }
        }
        
        std::cout << "Screen buffer composition: " 
                  << white_pixels << " white pixels, "
                  << non_white_pixels << " non-white pixels" << std::endl;
                  
        // If we have content, dump the first few non-white pixel values
        if (non_white_pixels > 0) {
            std::cout << "Sample non-white pixels (hex): ";
            int samples = 0;
            for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT && samples < 5; i++) {
                if (screen_buffer[i] != 0xFFFFFFFF) {
                    std::cout << std::hex << "0x" << screen_buffer[i] << " ";
                    samples++;
                }
            }
            std::cout << std::dec << std::endl;
        }
    }
}

void GPU::checkLYC() {
    uint8_t ly = memory.read(LY_REG);
    uint8_t lyc = memory.read(LYC_REG);
    uint8_t stat = memory.read(STAT_REG);
    
    // Check if LY=LYC
    if (ly == lyc) {
        // Set coincidence flag
        stat |= 0x04;
        
        // Trigger LYC=LY STAT interrupt if enabled
        if (stat & 0x40) {
            if (lcd_stat_callback) {
                lcd_stat_callback();
            }
        }
    } else {
        // Clear coincidence flag
        stat &= ~0x04;
    }
    
    // Update STAT register
    memory.write(STAT_REG, stat);
}

void GPU::renderBackground(int scanline) {
    // This method is now deprecated in favor of the FIFO implementation
    // But kept for reference
}

void GPU::renderWindow(int scanline) {
    // This method is now deprecated in favor of the FIFO implementation
    // But kept for reference
}

void GPU::renderSprites(int scanline) {
    // This method is now deprecated in favor of the FIFO implementation
    // But kept for reference
}

void GPU::checkVRAMData() {
    // Check if VRAM contains valid tile data
    bool has_data = false;
    int data_count = 0;
    int total_bytes_checked = 0;
    
    std::cout << "\n=== VRAM DATA CHECK (Frame " << frame_counter << ") ===" << std::endl;
    
    // Check the first 16KB of VRAM for non-zero data
    for (int addr = 0x8000; addr < 0x9800; addr += 64) {
        total_bytes_checked++;
        uint8_t sample = memory.read(addr);
        if (sample != 0x00) {
            data_count++;
            if (!has_data) { // Log the first occurrence
                std::cout << "Found non-zero VRAM data at 0x" << std::hex << addr 
                          << ": 0x" << static_cast<int>(sample) << std::dec << std::endl;
            has_data = true;
            }
        }
    }
    
    // Calculate percentage of VRAM with data
    float data_percentage = (data_count / static_cast<float>(total_bytes_checked)) * 100.0f;
    
    std::cout << "VRAM data check: " << data_count << " of " << total_bytes_checked 
              << " sampled locations contain non-zero data (" 
              << std::fixed << std::setprecision(2) << data_percentage << "%)" << std::endl;
    
    // If no data is found, we'll use a debug pattern
    if (!has_data) {
        using_debug_pattern = true;
        std::cout << "No valid data found in VRAM, using debug pattern for display" << std::endl;
    } else {
        using_debug_pattern = false;
        std::cout << "VRAM contains valid data, using game graphics for display" << std::endl;
    }
    
    std::cout << "=== END VRAM DATA CHECK ===\n" << std::endl;
}

uint8_t GPU::getColorFromPalette(uint8_t palette, uint8_t colorIdx) {
    // Extract the color value (0-3) from the palette data
    // Each palette is 8 bits with 4 colors (2 bits each)
    // Color 0 is bits 0-1, color 1 is bits 2-3, etc.
    uint8_t colorValue = (palette >> (colorIdx * 2)) & 0x03;
    
    // Debug output - drastically limit to avoid spamming console
    static int debug_count = 0;
    if (debug_count++ % 1000000 == 0) {
        std::cout << "Palette mapping: index " << static_cast<int>(colorIdx) 
                  << " maps to color " << static_cast<int>(colorValue)
                  << " (palette=0x" << std::hex << static_cast<int>(palette) << std::dec << ")" 
                  << std::endl;
    }
    
    return colorValue;
}

uint32_t GPU::getRGBColor(uint8_t colorValue) {
    // Convert Game Boy color value (0-3) to ARGB8888
    uint32_t color;
    
    // Use extremely distinct colors to diagnose rendering issues
    switch (colorValue) {
        case 0: 
            color = 0xFFFF0000; // Bright RED for color 0
            break;
        case 1: 
            color = 0xFF00FF00; // Bright GREEN for color 1
            break;
        case 2: 
            color = 0xFF0000FF; // Bright BLUE for color 2
            break;
        case 3: 
            color = 0xFFFFFFFF; // WHITE for color 3
            break;
        default: 
            color = 0xFFFF00FF; // Magenta (error)
            break;
    }
    
    // Debug output - drastically limit to avoid spamming console
    static int color_debug_count = 0;
    if (color_debug_count++ % 1000000 == 0) {
        std::cout << "RGB Color mapping: GB color " << static_cast<int>(colorValue) 
                  << " maps to RGB 0x" << std::hex << color << std::dec << std::endl;
    }
    
    return color;
}

void GPU::reset() {
    // Reset screen buffer to white
    screen_buffer.resize(SCREEN_WIDTH * SCREEN_HEIGHT, 0xFFFFFFFF);
    
    // Reset PPU state
    current_mode = LCDMode::HBLANK;
    mode_cycles = 0;
    
    // Reset LCD registers to power-on values
    memory.write(LCDC_REG, 0x91);  // LCD & BG enabled
    memory.write(STAT_REG, 0x00);  // Mode 0, no interrupts enabled
    memory.write(SCY_REG, 0x00);   // Scroll Y = 0
    memory.write(SCX_REG, 0x00);   // Scroll X = 0
    memory.write(LY_REG, 0x00);    // Current scanline = 0
    memory.write(LYC_REG, 0x00);   // LY Compare = 0
    memory.write(BGP_REG, 0xE4);   // BG palette: 3-2-1-0 (black, dark gray, light gray, white)
    memory.write(OBP0_REG, 0xE4);  // OBJ Palette 0: same as BG
    memory.write(OBP1_REG, 0xE4);  // OBJ Palette 1: same as BG
    memory.write(WY_REG, 0x00);    // Window Y = 0
    memory.write(WX_REG, 0x00);    // Window X = 0
    
    // Reset other state
    line = 0;
    frame_counter = 0;
    using_debug_pattern = true;
    cycles_since_last_debug = 0;
    window_line = 0;
    
    // Reset FIFO state
    bg_fifo.clear();
    sprite_fifo.clear();
    fifo_x = 0;
    pixel_x = 0;
    fetcher_x = 0;
    fetcher_state = FetcherState::TILE;
    window_active = false;
    
    // Force debug pattern again after reset
    // Create a simple checkerboard pattern in the first few tiles
    for (int tile = 0; tile < 256; tile++) {
        for (int y = 0; y < 8; y++) {
            uint8_t pattern = (y % 2) ? 0xAA : 0x55;
            // Each row is defined by 2 bytes (low/high bits)
            vram[tile * 16 + y * 2] = pattern;
            vram[tile * 16 + y * 2 + 1] = pattern;
        }
    }
    
    // Fill tilemap with non-zero tile indices
    for (int i = 0; i < 32 * 32; i++) {
        vram[0x1800 + i] = 1; // Use tile #1 for the background map
        vram[0x1C00 + i] = 2; // Use tile #2 for the window map
    }
    
    std::cout << "GPU reset completed with test pattern" << std::endl;
}

// Calculate Mode 3 duration based on the current scanline
uint16_t GPU::calculateMode3Duration(uint8_t scanline) {
    // Base duration is 172 dots
    uint16_t duration = 172;
    
    // Get LCDC register
    uint8_t lcdc = memory.read(LCDC_REG);
    
    // Get scroll X value
    uint8_t scrollX = memory.read(SCX_REG);
    
    // Add penalty for scrollX % 8
    duration += (scrollX % 8);
    
    // Check if window is enabled and visible on this scanline
    if ((lcdc & 0x20) && scanline >= memory.read(WY_REG)) {
        uint8_t wx = memory.read(WX_REG);
        if (wx >= 0 && wx <= 166) {
            // Window is visible, add penalty
            duration += 6;
        }
    }
    
    // Count sprites on this scanline and add penalties
    // This is a simplification - accurate emulation would be more complex
    uint8_t spriteHeight = (lcdc & 0x04) ? 16 : 8;
    int spritesOnLine = 0;
    
    for (int i = 0; i < 40 && spritesOnLine < 10; i++) {
        uint16_t oamAddr = 0xFE00 + (i * 4);
        uint8_t spriteY = memory.read(oamAddr) - 16;
        
        if (scanline >= spriteY && scanline < spriteY + spriteHeight) {
            spritesOnLine++;
            // Add approximate sprite penalty
            duration += 6;
        }
    }
    
    return duration;
}

void GPU::dumpVRAM(const std::string& filename) {
    // Debug function to dump VRAM to a file
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for VRAM dump: " << filename << std::endl;
        return;
    }
    
    // Get VRAM data via memory bus
    file << "VRAM Contents:" << std::endl;
    for (int addr = 0x8000; addr < 0xA000; addr++) {
        if ((addr - 0x8000) % 16 == 0) {
            file << std::hex << "0x" << addr << ": ";
        }
        
        file << std::hex << static_cast<int>(memory.read(addr)) << " ";
        
        if ((addr - 0x8000) % 16 == 15) {
            file << std::endl;
        }
    }
    
    std::cout << "VRAM dump written to " << filename << std::endl;
}

void GPU::forceVRAMCheck() {
    // Debug function to force VRAM check
    std::cout << "Force VRAM check called" << std::endl;
    checkVRAMData();
}

// Add this new function to draw a test pattern directly to the screen buffer
void GPU::drawTestPattern() {
    // Create a clearly visible checkerboard pattern with impossible-to-miss colors
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            // Large 8x8 pixel squares
            bool isOddTile = ((x / 8) % 2) != ((y / 8) % 2);
            
            // Choose extreme colors that would be impossible to miss
            uint32_t color;
            if (isOddTile) {
                // Pure Red for odd tiles
                color = 0xFFFF0000; 
            } else {
                // Pure Blue for even tiles
                color = 0xFF0000FF;
            }
            
            // Draw a diagonal pattern with pure green
            if (x < 32 && y < 32 && x == y) {
                color = 0xFF00FF00; // Pure Green diagonal
            }
            
            // Add some shocking pink areas that would be impossible to miss
            if ((x < 16 && y < 16) || (x > SCREEN_WIDTH-16 && y > SCREEN_HEIGHT-16)) {
                color = 0xFFFF00FF; // Shocking pink in corners
            }
            
            // Set the pixel in the screen buffer
            screen_buffer[y * SCREEN_WIDTH + x] = color;
        }
    }
    
    // Debug: Make absolutely sure we're writing to the buffer
    std::cout << "TEST PATTERN: Filling screen buffer with extreme test pattern" << std::endl;
    std::cout << "Buffer address: " << &screen_buffer[0] << std::endl;
    std::cout << "Buffer size: " << screen_buffer.size() << " pixels" << std::endl;
    std::cout << "First 4 pixels (hex): " 
              << std::hex << "0x" << screen_buffer[0] << " "
              << "0x" << screen_buffer[1] << " "
              << "0x" << screen_buffer[2] << " "
              << "0x" << screen_buffer[3] << std::dec << std::endl;
}

// Add a new method for tilemap debugging
void GPU::dumpTilemapDebug() {
    std::cout << "\n=== TILEMAP DEBUG INFO (Frame " << frame_counter << ") ===" << std::endl;
    
    // Display the first few entries of the background tile map
    uint16_t bg_tilemap_addr = getBackgroundTileMap();
    std::cout << "Background Tilemap (0x" << std::hex << bg_tilemap_addr << "):" << std::endl;
    
    // Count non-zero tiles
    int non_zero_tiles = 0;
    
    // Sample the first row of the tilemap
    std::cout << "First row: ";
    for (int x = 0; x < 16; x++) {
        uint8_t tile_idx = memory.read(bg_tilemap_addr + x);
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(tile_idx) << " ";
        
        if (tile_idx != 0) {
            non_zero_tiles++;
        }
    }
    std::cout << std::endl;
    
    // Sample the middle row
    std::cout << "Middle row: ";
    for (int x = 0; x < 16; x++) {
        uint8_t tile_idx = memory.read(bg_tilemap_addr + 16*32 + x);
        std::cout << std::hex << std::setw(2) << std::setfill('0') 
                  << static_cast<int>(tile_idx) << " ";
        
        if (tile_idx != 0) {
            non_zero_tiles++;
        }
    }
    std::cout << std::endl;
    
    // Count all non-zero tiles in the tilemap
    for (int i = 0; i < 32*32; i++) {
        if (memory.read(bg_tilemap_addr + i) != 0) {
            non_zero_tiles++;
        }
    }
    
    std::cout << "Total non-zero tiles in background map: " << std::dec << non_zero_tiles << std::endl;
    
    // Now check tile data
    std::cout << "Tile Data (First 5 tiles):" << std::endl;
    uint16_t tile_data_addr = getTileDataAddress();
    
    for (int tile = 0; tile < 5; tile++) {
        std::cout << "Tile #" << std::dec << tile << ": ";
        bool has_data = false;
        
        for (int byte = 0; byte < 16; byte++) {
            uint8_t data = memory.read(tile_data_addr + tile * 16 + byte);
            if (data != 0) {
                has_data = true;
            }
            
            if (byte < 4) { // Just show first 4 bytes
                std::cout << std::hex << std::setw(2) << std::setfill('0') 
                          << static_cast<int>(data) << " ";
            }
        }
        
        std::cout << (has_data ? "[HAS DATA]" : "[EMPTY]") << std::endl;
    }
    
    // Check LCDC register for display state
    uint8_t lcdc = memory.read(LCDC_REG);
    std::cout << "LCDC: 0x" << std::hex << static_cast<int>(lcdc) << std::dec 
              << " (LCD: " << (isLCDEnabled() ? "ON" : "OFF")
              << ", BG: " << (areBGAndWindowEnabled() ? "ON" : "OFF")
              << ", WIN: " << (isWindowEnabled() ? "ON" : "OFF")
              << ", SPRITES: " << (areSpritesEnabled() ? "ON" : "OFF")
              << ")" << std::endl;
              
    // Check scroll positions
    std::cout << "Scroll: X=" << static_cast<int>(memory.read(SCX_REG)) 
              << ", Y=" << static_cast<int>(memory.read(SCY_REG)) << std::endl;
    
    std::cout << "=== END TILEMAP DEBUG INFO ===\n" << std::endl;
}

void GPU::dumpVRAMDebug() {
    std::ofstream file("vram_debug.txt");
    if (!file) {
        std::cerr << "Error: Could not open vram_debug.txt for writing" << std::endl;
        return;
    }

    // Dump the tile data (0x8000-0x97FF)
    file << "TILE DATA (0x8000-0x97FF):" << std::endl;
    file << "=========================" << std::endl;
    for (int tile = 0; tile < 384; tile++) {
        file << "Tile #" << tile << " at VRAM offset 0x" << std::hex << (tile * 16) << std::dec << ":" << std::endl;
        
        // Each tile is 8x8 pixels, with 2 bits per pixel (16 bytes total)
        for (int y = 0; y < 8; y++) {
            uint8_t low_byte = vram[(tile * 16) + (y * 2)];
            uint8_t high_byte = vram[(tile * 16) + (y * 2) + 1];
            
            file << "  ";
            // Reconstruct the row of pixels
            for (int x = 7; x >= 0; x--) {
                int color_idx = ((high_byte >> x) & 1) << 1 | ((low_byte >> x) & 1);
                file << color_idx << " ";
            }
            file << std::endl;
        }
        file << std::endl;
    }
    
    // Dump the background tile map (0x9800-0x9BFF)
    file << "BACKGROUND TILE MAP (0x9800-0x9BFF):" << std::endl;
    file << "=================================" << std::endl;
    for (int y = 0; y < 32; y++) {
        file << "Row " << y << ": ";
        for (int x = 0; x < 32; x++) {
            int map_offset = 0x1800 + (y * 32) + x; // 0x9800 - 0x8000 = 0x1800
            file << std::hex << std::setw(2) << std::setfill('0') 
                 << (int)vram[map_offset] << " ";
        }
        file << std::dec << std::endl;
    }
    
    // Dump the window tile map (0x9C00-0x9FFF)
    file << "WINDOW TILE MAP (0x9C00-0x9FFF):" << std::endl;
    file << "=============================" << std::endl;
    for (int y = 0; y < 32; y++) {
        file << "Row " << y << ": ";
        for (int x = 0; x < 32; x++) {
            int map_offset = 0x1C00 + (y * 32) + x; // 0x9C00 - 0x8000 = 0x1C00
            file << std::hex << std::setw(2) << std::setfill('0') 
                 << (int)vram[map_offset] << " ";
        }
        file << std::dec << std::endl;
    }
    
    // Dump LCD and palette registers
    file << "REGISTER VALUES:" << std::endl;
    file << "================" << std::endl;
    file << "LCDC: 0x" << std::hex << (int)memory.read(LCDC_REG) << std::endl;
    file << "STAT: 0x" << std::hex << (int)memory.read(STAT_REG) << std::endl;
    file << "SCY: 0x" << std::hex << (int)memory.read(SCY_REG) << std::endl;
    file << "SCX: 0x" << std::hex << (int)memory.read(SCX_REG) << std::endl;
    file << "LY: 0x" << std::hex << (int)memory.read(LY_REG) << std::endl;
    file << "LYC: 0x" << std::hex << (int)memory.read(LYC_REG) << std::endl;
    file << "BGP: 0x" << std::hex << (int)memory.read(BGP_REG) << std::endl;
    file << "OBP0: 0x" << std::hex << (int)memory.read(OBP0_REG) << std::endl;
    file << "OBP1: 0x" << std::hex << (int)memory.read(OBP1_REG) << std::endl;
    file << "WY: 0x" << std::hex << (int)memory.read(WY_REG) << std::endl;
    file << "WX: 0x" << std::hex << (int)memory.read(WX_REG) << std::endl;
    
    file.close();
    std::cout << "VRAM debug info written to vram_debug.txt" << std::endl;
}
