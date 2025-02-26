#include "cpu.hpp"
#include "instructions.hpp"
#include <stdio.h>

// Helper functions for register access
uint8_t CPU::getRegister8Bit(Instructions::RegType reg) {
    switch (reg) {
        case Instructions::RegType::A: return registers.a;
        case Instructions::RegType::B: return registers.b;
        case Instructions::RegType::C: return registers.c;
        case Instructions::RegType::D: return registers.d;
        case Instructions::RegType::E: return registers.e;
        case Instructions::RegType::H: return registers.h;
        case Instructions::RegType::L: return registers.l;
        case Instructions::RegType::F: return registers.f;
        default: 
            printf("Invalid 8-bit register access\n");
            return 0;
    }
}

void CPU::setRegister8Bit(Instructions::RegType reg, uint8_t value) {
    switch (reg) {
        case Instructions::RegType::A: registers.a = value; break;
        case Instructions::RegType::B: registers.b = value; break;
        case Instructions::RegType::C: registers.c = value; break;
        case Instructions::RegType::D: registers.d = value; break;
        case Instructions::RegType::E: registers.e = value; break;
        case Instructions::RegType::H: registers.h = value; break;
        case Instructions::RegType::L: registers.l = value; break;
        case Instructions::RegType::F: registers.f = value & 0xF0; break; // Only upper 4 bits used
        default: printf("Invalid 8-bit register access\n"); break;
    }
}

uint16_t CPU::getRegister16Bit(Instructions::RegType reg) {
    switch (reg) {
        case Instructions::RegType::AF: return registers.af;
        case Instructions::RegType::BC: return registers.bc;
        case Instructions::RegType::DE: return registers.de;
        case Instructions::RegType::HL: return registers.hl;
        case Instructions::RegType::SP: return registers.sp;
        case Instructions::RegType::PC: return registers.pc;
        default: 
            printf("Invalid 16-bit register access\n");
            return 0;
    }
}

void CPU::setRegister16Bit(Instructions::RegType reg, uint16_t value) {
    switch (reg) {
        case Instructions::RegType::AF: registers.af = value & 0xFFF0; break; // Lower 4 bits of F always 0
        case Instructions::RegType::BC: registers.bc = value; break;
        case Instructions::RegType::DE: registers.de = value; break;
        case Instructions::RegType::HL: registers.hl = value; break;
        case Instructions::RegType::SP: registers.sp = value; break;
        case Instructions::RegType::PC: registers.pc = value; break;
        default: printf("Invalid 16-bit register access\n"); break;
    }
}

bool CPU::isRegister16Bit(Instructions::RegType reg) {
    return reg == Instructions::RegType::AF || 
           reg == Instructions::RegType::BC || 
           reg == Instructions::RegType::DE || 
           reg == Instructions::RegType::HL || 
           reg == Instructions::RegType::SP || 
           reg == Instructions::RegType::PC;
} 