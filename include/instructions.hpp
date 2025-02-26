#pragma once
#include <cstdint>
#include <string>
#include <array>
#include <memory>

class Instructions {
public:
    // Addressing modes
    enum class AddrMode {
        IMP,        // Implied
        R_D16,      // Register with 16-bit immediate
        R_R,        // Register to Register
        MR_R,       // Memory (Register) to Register
        R,          // Register
        R_D8,       // Register with 8-bit immediate
        R_MR,       // Register to Memory (Register)
        R_HLI,      // Register to HL, increment
        R_HLD,      // Register to HL, decrement
        HLI_R,      // HL to Register, increment
        HLD_R,      // HL to Register, decrement
        R_A8,       // Register to 8-bit address
        A8_R,       // 8-bit address to Register
        HL_SPR,     // HL to SP + relative
        D16,        // 16-bit immediate
        D8,         // 8-bit immediate
        D16_R,      // 16-bit immediate to Register
        MR_D8,      // Memory (Register) with 8-bit immediate
        MR,         // Memory (Register)
        A16_R,      // 16-bit address to Register
        R_A16,      // Register to 16-bit address
        CC_D16,     // Conditional jump/call with 16-bit immediate
        CC_D8,      // Conditional relative jump with 8-bit immediate
        CC          // Conditional return
    };

    // Register types
    enum class RegType {
        NONE,
        A, F, B, C, D, E, H, L,
        AF, BC, DE, HL,
        SP, PC,
        CC_NZ,   // Condition: Not Zero
        CC_Z,    // Condition: Zero
        CC_NC,   // Condition: Not Carry
        CC_C     // Condition: Carry
    };

    // Instruction types
    enum class Type {
        NONE,
        NOP, LD, INC, DEC,
        RLCA, ADD, RRCA, STOP,
        RLA, JR, RRA, DAA,
        CPL, SCF, CCF, HALT,
        ADC, SUB, SBC, AND,
        XOR, OR, CP, POP,
        JP, PUSH, RET, CB,
        CALL, RETI, LDH,
        DI, EI, RST, ERR,
        // CB instructions
        RLC, RRC, RL, RR,
        SLA, SRA, SWAP, SRL,
        BIT, RES, SET
    };

    // Condition types
    enum class CondType {
        NONE,
        NZ,     // Not Zero
        Z,      // Zero
        NC,     // Not Carry
        C       // Carry
    };

    struct Instruction {
        Type type;
        AddrMode addr_mode;
        RegType reg1;
        RegType reg2;
        CondType cond;
        uint8_t param;

        Instruction(Type t = Type::NONE,
                   AddrMode am = AddrMode::IMP,
                   RegType r1 = RegType::NONE,
                   RegType r2 = RegType::NONE,
                   CondType c = CondType::NONE,
                   uint8_t p = 0)
            : type(t), addr_mode(am), reg1(r1), reg2(r2), cond(c), param(p) {}
    };

public:
    Instructions();  // Constructor will initialize instruction table
    
    // Get instruction by opcode
    const Instruction& get(uint8_t opcode) const;
    const Instruction& getCB(uint8_t opcode) const;  // For CB-prefixed instructions
    
    // Utility functions
    static std::string get_type_name(Type type);
    static std::string get_reg_name(RegType reg);
    static uint8_t get_reg_size(RegType reg);  // Returns 8 or 16 for register size
    
private:
    void initialize_instructions();  // Called by constructor to set up tables
    void initialize_cb_instructions();
    
    std::array<Instruction, 256> instructions;     // Main instruction table
    std::array<Instruction, 256> cb_instructions;  // CB-prefixed instruction table
};

