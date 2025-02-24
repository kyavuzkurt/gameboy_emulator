#include "cpu.hpp"
#include "instructions.hpp"
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

void CPU::execute() {
    printf("Executing instruction 0x%02X at PC: 0x%04X - %s\n", 
           current_opcode, 
           registers.pc,
           Instructions::get_type_name(current_instruction->type).c_str());
    
    SDL_Delay(100); //Debug
    
    // TODO: Actually execute the instruction based on type
    switch (current_instruction->type) {
        case Instructions::Type::NOP:
            // Do nothing
            break;
        case Instructions::Type::JP:
            // Jump to a new location
            
            break;
        // Add other instruction implementations
        default:
            printf("Unimplemented instruction: %s\n", 
                   Instructions::get_type_name(current_instruction->type).c_str());
            break;
    }
}
