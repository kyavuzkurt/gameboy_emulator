#pragma once
#include "memory.hpp"
#include "instructions.hpp"
#include <cstdint>

class CPU {
public:
    explicit CPU(MemoryBus& memory);
    
    void tick(); // Execute one CPU cycle
    
    uint16_t getPC() const { return registers.pc; } 
    
private:
    // Register structure
    struct Registers {
        // Main registers
        union {
            struct {
                uint8_t f; // Flag register
                uint8_t a; // Accumulator
            };
            uint16_t af;
        };
        
        union {
            struct {
                uint8_t c;
                uint8_t b;
            };
            uint16_t bc;
        };
        
        union {
            struct {
                uint8_t e;
                uint8_t d;
            };
            uint16_t de;
        };
        
        union {
            struct {
                uint8_t l;
                uint8_t h;
            };
            uint16_t hl;
        };
        
        uint16_t sp; // Stack pointer
        uint16_t pc; // Program counter
    } registers;
    
    // Flag register bits
    static constexpr uint8_t FLAG_Z = 0x80; // Zero flag
    static constexpr uint8_t FLAG_N = 0x40; // Subtract flag
    static constexpr uint8_t FLAG_H = 0x20; // Half carry flag
    static constexpr uint8_t FLAG_C = 0x10; // Carry flag
    
    // Flag operations
    bool getFlag(uint8_t flag) const { return (registers.f & flag) != 0; }
    void setFlag(uint8_t flag, bool value) {
        if (value) registers.f |= flag;
        else registers.f &= ~flag;
    }
    
    MemoryBus& memory;
    uint8_t current_opcode;
    
    // CPU state
    bool halted = false;
    bool stopped = false;
    

    // Instruction handling
    Instructions instructions;  // Add this to store the instruction set
    const Instructions::Instruction* current_instruction;  // Current instruction being executed
    
    // Fetch-decode-execute cycle
    void fetch_instruction();  // Fetch next instruction
    void fetch_adress(); // Fetch the adress data
    void execute(); // Execute the instruction
};
