#include "cpu.hpp"
#include "instructions.hpp"
#include "memory.hpp"
#include <stdio.h>
#include <SDL2/SDL.h>

CPU::CPU(MemoryBus& mem) : memory(mem) {
    // Initialize registers to their power-up values
    registers = {};
    registers.af = 0x01B0;
    registers.bc = 0x0013;
    registers.de = 0x00D8;
    registers.hl = 0x014D;
    registers.sp = 0xFFFE;
    registers.pc = 0x0100; // Start execution at 0x0100
}

void CPU::tick() {
    if (!halted && !stopped) {
        fetch_instruction();
        fetch_adress();
        execute();
    }
}

void CPU::fetch_instruction(){
    current_opcode = memory.read(registers.pc);
    current_instruction = &instructions.get(current_opcode);
}

void CPU::fetch_adress(){
    // First, increment PC to point to the next byte after opcode
    registers.pc++;
    
    // Fetch additional bytes based on addressing mode
    switch (current_instruction->addr_mode) {
        case Instructions::AddrMode::IMP:
            // Implied addressing - no additional bytes needed
            break;
            
        case Instructions::AddrMode::R_D8:
        case Instructions::AddrMode::MR_D8:
        case Instructions::AddrMode::D8:
            // 8-bit immediate data
            current_instruction_data.immediate_value = memory.read(registers.pc);
            registers.pc++;
            break;
            
        case Instructions::AddrMode::R_D16:
        case Instructions::AddrMode::D16:
        case Instructions::AddrMode::D16_R:
        case Instructions::AddrMode::A16_R:
        case Instructions::AddrMode::R_A16:
            // 16-bit immediate data (little endian)
            current_instruction_data.immediate_value = memory.read(registers.pc);
            registers.pc++;
            current_instruction_data.immediate_value |= (memory.read(registers.pc) << 8);
            registers.pc++;
            break;
            
        case Instructions::AddrMode::R_A8:
        case Instructions::AddrMode::A8_R:
            // High RAM address ($FF00 + 8-bit immediate)
            current_instruction_data.immediate_value = memory.read(registers.pc);
            registers.pc++;
            break;
            
        default:
            // Other addressing modes don't need additional bytes
            break;
    }
}

void CPU::execute() {
    // Debug section start
    printf("Executing: 0x%02X (%s) at PC: 0x%04X | ", 
           current_opcode, 
           Instructions::get_type_name(current_instruction->type).c_str(),
           registers.pc);
    
    // Print addressing mode and registers involved
    printf("Mode: %d | ", static_cast<int>(current_instruction->addr_mode));
    
    if (current_instruction->reg1 != Instructions::RegType::NONE) {
        printf("Reg1: %s ", Instructions::get_reg_name(current_instruction->reg1).c_str());
    }
    
    if (current_instruction->reg2 != Instructions::RegType::NONE) {
        printf("Reg2: %s ", Instructions::get_reg_name(current_instruction->reg2).c_str());
    }
    
    // Print immediate value if applicable
    if (current_instruction->addr_mode == Instructions::AddrMode::R_D8 || 
        current_instruction->addr_mode == Instructions::AddrMode::D8 ||
        current_instruction->addr_mode == Instructions::AddrMode::MR_D8) {
        printf("| Imm: 0x%02X ", current_instruction_data.immediate_value & 0xFF);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D16 || 
               current_instruction->addr_mode == Instructions::AddrMode::D16 ||
               current_instruction->addr_mode == Instructions::AddrMode::D16_R) {
        printf("| Imm: 0x%04X ", current_instruction_data.immediate_value);
    }
    
    // Print register values
    printf("\n  Registers: A:0x%02X F:0x%02X B:0x%02X C:0x%02X D:0x%02X E:0x%02X H:0x%02X L:0x%02X SP:0x%04X",
           registers.a, registers.f, registers.b, registers.c, 
           registers.d, registers.e, registers.h, registers.l, registers.sp);
    
    // Print flags
    printf("\n  Flags: Z:%d N:%d H:%d C:%d", 
           getFlag(FLAG_Z) ? 1 : 0, 
           getFlag(FLAG_N) ? 1 : 0, 
           getFlag(FLAG_H) ? 1 : 0, 
           getFlag(FLAG_C) ? 1 : 0);
    
    // Print 16-bit register pairs
    printf("\n  Pairs: AF:0x%04X BC:0x%04X DE:0x%04X HL:0x%04X\n", 
           registers.af, registers.bc, registers.de, registers.hl);
    
    //Debug section end
    // Execute the instruction based on type
    switch (current_instruction->type) {
        case Instructions::Type::NOP:
            // No Operation - does nothing
            break;
            
        case Instructions::Type::LD:
            // Load instruction - handle different addressing modes
            executeLD();
            break;
            
        case Instructions::Type::INC:
            // Increment register or memory
            executeINC();
            break;
            
        case Instructions::Type::DEC:
            // Decrement register or memory
            executeDEC();
            break;
            
        case Instructions::Type::ADD:
            // Addition
            executeADD();
            break;
            
        case Instructions::Type::SUB:
            // Subtraction
            executeSUB();
            break;
            
        case Instructions::Type::AND:
            // Logical AND
            executeAND();
            break;
            
        case Instructions::Type::OR:
            // Logical OR
            executeOR();
            break;
            
        case Instructions::Type::XOR:
            // Logical XOR
            executeXOR();
            break;
            
        case Instructions::Type::JP:
            // Jump
            executeJP();
            break;
            
        case Instructions::Type::JR:
            // Jump relative
            executeJR();
            break;
            
        case Instructions::Type::CALL:
            // Call subroutine
            executeCALL();
            break;
            
        case Instructions::Type::RET:
            // Return from subroutine
            executeRET();
            break;
            
        case Instructions::Type::PUSH:
            // Push register pair to stack
            executePUSH();
            break;
            
        case Instructions::Type::POP:
            // Pop register pair from stack
            executePOP();
            break;
            
        case Instructions::Type::RLCA:
            // Rotate Left Circular Accumulator
            executeRLCA();
            break;
        case Instructions::Type::RRCA:
            // Rotate Right Circular Accumulator
            executeRRCA();
            break;
        case Instructions::Type::STOP:
            // Stop
            // Technically this isn't correct but it works.
            break;
        case Instructions::Type::RLA:
            // Rotate Left Accumulator
            executeRLA();
            break;
        case Instructions::Type::RRA:
            // Rotate Right Accumulator
            executeRRA();
            break;
        case Instructions::Type::DAA:
            // Decimal Adjust Acccumulator
            executeDAA();
            break;
        case Instructions::Type::CPL:
            // Complement
            executeCPL();
            break;
        case Instructions::Type::SCF:
            // Set Carry Flag
            executeSCF();
            break;
        case Instructions::Type::CCF:
            // Complement Carry Flag
            executeCCF();
            break;
        case Instructions::Type::HALT:
            // Halt
            executeHALT();
            break;
        case Instructions::Type::ADC:
            // Add with Carry
            executeADC();
            break;
        case Instructions::Type::SBC:
            // Subtract with Carry
            executeSBC();
            break;
        case Instructions::Type::CP:
            // Compare
            executeCP();
            break;
        case Instructions::Type::RETI:
            // Return from Interrupt
            executeRETI();
            break;
        case Instructions::Type::LDH:
            // Load High
            executeLDH();
            break;
        case Instructions::Type::DI:
            // Disable Interrupts
            executeDI();
            break;
        case Instructions::Type::EI:
            // Enable Interrupts
            executeEI();
            break;
        case Instructions::Type::RST:
            // Reset
            executeRST();
            break;
        case Instructions::Type::ERR:
            // Error 
            break;

        // CB Instructions
        case Instructions::Type::RLC:
            // Rotate Left Circular
            executeRLC();
            break;
        case Instructions::Type::RRC:
            // Rotate Right Circular
            executeRRC();
            break;
        case Instructions::Type::RL:
            // Rotate Left through carry
            executeRL();
            break;
        case Instructions::Type::RR:
            // Rotate Right through carry
            executeRR();
            break;
        case Instructions::Type::SLA:
            // Shift Left Arithmetic
            executeSLA();
            break;
        case Instructions::Type::SRA:
            // Shift Right Arithmetic
            executeSRA();
            break;
        case Instructions::Type::SWAP:
            // Swap nibbles
            executeSWAP();
            break;
        case Instructions::Type::SRL:
            // Shift Right Logical
            executeSRL();
            break;
        case Instructions::Type::BIT:
            // Test bit
            executeBIT();
            break;
        case Instructions::Type::RES:
            // Reset bit
            executeRES();
            break;
        case Instructions::Type::SET:
            // Set bit
            executeSET();
            break;

        default:
            printf("Unimplemented instruction: %s\n", 
                   Instructions::get_type_name(current_instruction->type).c_str());
            break;
    }
}
