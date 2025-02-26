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

    struct InstructionData {
        uint16_t immediate_value = 0;  // For immediate values and addresses
    } current_instruction_data;



    // Helper methods for register access
    uint8_t getRegister8Bit(Instructions::RegType reg);
    void setRegister8Bit(Instructions::RegType reg, uint8_t value);
    uint16_t getRegister16Bit(Instructions::RegType reg);
    void setRegister16Bit(Instructions::RegType reg, uint16_t value);
    bool isRegister16Bit(Instructions::RegType reg);

    void executeLD();
    void executeINC();
    void executeDEC();
    void executeADD();
    void executeSUB();
    void executeAND();
    void executeOR();
    void executeXOR();
    void executeJP();
    void executeJR();
    void executeCALL();
    void executeRET();
    void executePUSH();
    void executePOP();
    void executeRLCA();
    void executeRRCA();
    void executeRLA();
    void executeRRA();
    void executeDAA();
    void executeCPL();
    void executeSCF();
    void executeCCF();
    void executeHALT();
    void executeADC();
    void executeSBC();
    void executeCP();
    void executeRETI();
    void executeLDH();
    void executeDI();
    void executeEI();
    void executeRST();
    void executeRLC();
    void executeRRC();
    void executeRL();
    void executeRR();
    void executeSLA();
    void executeSRA();
    void executeSWAP();
    void executeSRL();
    void executeBIT();
    void executeRES();
    void executeSET();

    bool ime = true; // Interrupt Master Enable flag
};
