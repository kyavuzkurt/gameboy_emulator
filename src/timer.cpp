#include "timer.hpp"
#include "memory.hpp"

// Timer register addresses
constexpr uint16_t DIV_REGISTER_ADDR = 0xFF04;
constexpr uint16_t TIMA_REGISTER_ADDR = 0xFF05;
constexpr uint16_t TMA_REGISTER_ADDR = 0xFF06;
constexpr uint16_t TAC_REGISTER_ADDR = 0xFF07;

// Interrupt flags address
constexpr uint16_t IF_REGISTER_ADDR = 0xFF0F;

// Timer interrupt bit in IF register
constexpr uint8_t TIMER_INTERRUPT_FLAG = 0x04;

Timer::Timer(MemoryBus& memory)
    : memory(memory), 
      div_counter(0), 
      div(0), 
      tima(0), 
      tma(0), 
      tac(0), 
      interrupt_requested(false),
      tima_reload_scheduled(false),
      previous_bit_state(false) {
}

void Timer::tick(uint8_t cycles) {
    // For each CPU M-cycle
    for (uint8_t i = 0; i < cycles; i++) {
        // Handle TIMA reload that was scheduled in the previous cycle
        if (tima_reload_scheduled) {
            tima = tma;
            tima_reload_scheduled = false;
            
            // Request timer interrupt
            interrupt_requested = true;
            
            // Set the timer interrupt flag in the IF register
            uint8_t if_value = memory.read(IF_REGISTER_ADDR);
            memory.write(IF_REGISTER_ADDR, if_value | TIMER_INTERRUPT_FLAG);
        }
        
        // Increment the system counter (DIV internal counter)
        div_counter++;
        
        // DIV register is the upper 8 bits of the 16-bit system counter
        div = div_counter >> 8;
        
        // Get the currently selected bit from the system counter based on TAC
        bool current_bit_state = false;
        
        if (isTimerEnabled()) {
            switch (tac & 0x03) {
                case 0: current_bit_state = (div_counter & (1 << 9)) != 0; break;  // 4096Hz (bit 9)
                case 1: current_bit_state = (div_counter & (1 << 3)) != 0; break;  // 262144Hz (bit 3)
                case 2: current_bit_state = (div_counter & (1 << 5)) != 0; break;  // 65536Hz (bit 5)
                case 3: current_bit_state = (div_counter & (1 << 7)) != 0; break;  // 16384Hz (bit 7)
            }
        }
        
        // Falling edge detector
        if (previous_bit_state && !current_bit_state) {
            // Increment TIMA on falling edge
            tima++;
            
            // Check for TIMA overflow
            if (tima == 0) {
                // Schedule TIMA reload for the next cycle
                tima_reload_scheduled = true;
            }
        }
        
        // Update previous state for next cycle's edge detection
        previous_bit_state = current_bit_state;
    }
}

uint8_t Timer::readRegister(uint16_t address) const {
    switch (address) {
        case DIV_REGISTER_ADDR:
            return div;
        case TIMA_REGISTER_ADDR:
            return tima;
        case TMA_REGISTER_ADDR:
            return tma;
        case TAC_REGISTER_ADDR:
            return tac | 0xF8;  // Upper bits are always set when read
        default:
            return 0xFF; // Should not happen if called correctly
    }
}

void Timer::writeRegister(uint16_t address, uint8_t value) {
    // Get the current bit state before any changes
    bool old_bit_state = false;
    if (isTimerEnabled()) {
        switch (tac & 0x03) {
            case 0: old_bit_state = (div_counter & (1 << 9)) != 0; break;
            case 1: old_bit_state = (div_counter & (1 << 3)) != 0; break;
            case 2: old_bit_state = (div_counter & (1 << 5)) != 0; break;
            case 3: old_bit_state = (div_counter & (1 << 7)) != 0; break;
        }
    }
    
    // Declare new_bit_state here so it's in scope for all case statements
    bool new_bit_state = false;
    
    switch (address) {
        case DIV_REGISTER_ADDR:
            // Writing any value to DIV resets it to 0
            div_counter = 0;
            div = 0;
            
            // Get the new bit state after DIV reset
            if (isTimerEnabled()) {
                switch (tac & 0x03) {
                    case 0: new_bit_state = (div_counter & (1 << 9)) != 0; break;
                    case 1: new_bit_state = (div_counter & (1 << 3)) != 0; break;
                    case 2: new_bit_state = (div_counter & (1 << 5)) != 0; break;
                    case 3: new_bit_state = (div_counter & (1 << 7)) != 0; break;
                }
            }
            
            // If we had a falling edge due to DIV reset
            if (old_bit_state && !new_bit_state) {
                tima++;
                if (tima == 0) {
                    tima_reload_scheduled = true;
                }
            }
            
            // Update previous state after the change
            previous_bit_state = new_bit_state;
            break;
            
        case TIMA_REGISTER_ADDR:
            // Writing to TIMA during the cycle it overflows cancels the overflow
            if (tima_reload_scheduled) {
                tima_reload_scheduled = false;
            }
            tima = value;
            break;
            
        case TMA_REGISTER_ADDR:
            tma = value;
            // If TMA is written during the cycle after TIMA overflows,
            // the new value is immediately copied to TIMA as well
            if (tima_reload_scheduled) {
                tima = value;
            }
            break;
            
        case TAC_REGISTER_ADDR:
            // Get what would be the new bit state after TAC change
            if ((value & 0x04) != 0) { // If timer would be enabled
                switch (value & 0x03) {
                    case 0: new_bit_state = (div_counter & (1 << 9)) != 0; break;
                    case 1: new_bit_state = (div_counter & (1 << 3)) != 0; break;
                    case 2: new_bit_state = (div_counter & (1 << 5)) != 0; break;
                    case 3: new_bit_state = (div_counter & (1 << 7)) != 0; break;
                }
            }
            
            // Only the lower 3 bits are used in TAC
            tac = value & 0x07;
            
            // If we had a falling edge due to TAC change
            if (old_bit_state && !new_bit_state) {
                tima++;
                if (tima == 0) {
                    tima_reload_scheduled = true;
                }
            }
            
            // Update previous state after the change
            previous_bit_state = new_bit_state;
            break;
    }
}

uint32_t Timer::getTimerFrequency() const {
    // TAC bits 0-1 determine the timer frequency
    switch (tac & 0x03) {
        case 0: return 4096;    // 4096 Hz (CPU clock / 1024)
        case 1: return 262144;  // 262144 Hz (CPU clock / 16)
        case 2: return 65536;   // 65536 Hz (CPU clock / 64)
        case 3: return 16384;   // 16384 Hz (CPU clock / 256)
        default: return 4096;   // Default case (should never happen)
    }
}

bool Timer::isTimerEnabled() const {
    // TAC bit 2 determines if the timer is enabled
    return (tac & 0x04) != 0;
} 