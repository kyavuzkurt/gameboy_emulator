#pragma once
#include <cstdint>

class MemoryBus;

class Timer {
public:
    explicit Timer(MemoryBus& memory);
    
    void tick(uint8_t cycles);
    
    // Timer registers
    uint8_t readRegister(uint16_t address) const;
    void writeRegister(uint16_t address, uint8_t value);
    
    // Timer interrupt checking
    bool isInterruptRequested() const { return interrupt_requested; }
    void clearInterruptRequest() { interrupt_requested = false; }
    
private:
    MemoryBus& memory;
    
    // Timer registers
    uint16_t div_counter;    // Internal counter for DIV register
    uint8_t div;             // Divider Register (0xFF04)
    uint8_t tima;            // Timer Counter (0xFF05)
    uint8_t tma;             // Timer Modulo (0xFF06)
    uint8_t tac;             // Timer Control (0xFF07)
    
    bool interrupt_requested;
    bool tima_reload_scheduled;  // Flag for delayed TIMA reload after overflow
    bool previous_bit_state;     // Previous timer input bit for edge detection
    
    // Helper methods
    uint32_t getTimerFrequency() const;
    bool isTimerEnabled() const;
}; 