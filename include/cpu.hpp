#pragma once
#include "memory.hpp"
#include "instructions.hpp"
#include <cstdint>

class CPU {
public:
    explicit CPU(MemoryBus& memory);
    
    void reset();  // Reset CPU to post-boot ROM state
    
    void tick(); // Execute one CPU cycle
    
    uint16_t getPC() const { return registers.pc; } 
    
    // Get the number of cycles that have elapsed
    uint64_t getCycles() const { return cycles; }
    
    // Reset cycle count (useful for timing specific events)
    void resetCycles() { cycles = 0; }
    
    // Add a debug flag
    bool debug_output_enabled = false;
    
    void setRegisterAF(uint16_t value) { registers.af = value; }
    void setRegisterBC(uint16_t value) { registers.bc = value; }
    void setRegisterDE(uint16_t value) { registers.de = value; }
    void setRegisterHL(uint16_t value) { registers.hl = value; }
    void setPC(uint16_t value) { registers.pc = value; }
    void setSP(uint16_t value) { registers.sp = value; }
    void setIME(bool value) { ime = value; }
    
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
    
    // Timing related fields
    uint64_t cycles = 0;         // Total cycles elapsed
    uint8_t pending_cycles = 0;  // Cycles remaining for current instruction
    
    MemoryBus& memory;
    uint8_t current_opcode = 0;  // Current executing opcode
    
    // CPU state
    bool halted = false;
    bool stopped = false;
    

    // Instruction handling
    Instructions instructions;  // Add this to store the instruction set
    const Instructions::Instruction* current_instruction = nullptr;  // Current instruction being executed
    
    // Fetch-decode-execute cycle
    void fetch_instruction();  // Fetch next instruction
    void fetch_adress(); // Fetch the adress data
    void execute(); // Execute the instruction

    struct InstructionData {
        uint16_t immediate_value = 0;  // For immediate values and addresses
    } current_instruction_data;

    // Method for handling interrupts
    bool handleInterrupts();

    // Helper methods for register access
    uint8_t getRegister8Bit(Instructions::RegType reg);
    void setRegister8Bit(Instructions::RegType reg, uint8_t value);
    uint16_t getRegister16Bit(Instructions::RegType reg);
    void setRegister16Bit(Instructions::RegType reg, uint16_t value);
    bool isRegister16Bit(Instructions::RegType reg);

    // Modified execution methods to return branch taken status
    bool executeJP();
    bool executeJR();
    bool executeCALL();
    bool executeRET();
    
    // Other execute methods (unchanged)
    void executeLD();
    void executeINC();
    void executeDEC();
    void executeADD();
    void executeSUB();
    void executeAND();
    void executeOR();
    void executeXOR();
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
    void executeSTOP();
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
    bool halt_bug_active = false; // Flag to track HALT bug state
    
    // Helper for computing instruction timing
    uint8_t get_instruction_cycles(const Instructions::Instruction* instr, bool branch_taken = false);
    
    // Helper to check conditions for conditional instructions
    bool checkCondition(Instructions::CondType cond);

    // Debug counter to track executed instructions
    uint64_t debug_instruction_count;
};
