#include "cpu.hpp"
#include "instructions.hpp"
#include "memory.hpp"
#include <stdio.h>
#include <SDL2/SDL.h>
#include <iostream>

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
    // If CPU is stopped, do nothing
    if (stopped) {
        cycles++;
        return;
    }
    
    // Process interrupts
    bool interrupt_handled = handleInterrupts();
    
    // If an interrupt was handled, we've already processed cycles
    if (interrupt_handled) {
        return;
    }
    
    // If halted, just increase cycles and return
    if (halted) {
        cycles++;
        return;
    }
    
    // If no pending instruction, fetch a new one
    if (pending_cycles == 0) {
        // Debug: Print info at specific addresses that are important for VRAM activity
        if (registers.pc == 0x0100) {
            std::cout << "CPU TRACE: Starting execution at entry point 0x0100" << std::endl;
        }
        else if (registers.pc == 0x0150) {
            std::cout << "CPU TRACE: Finished boot sequence, jumping to actual game code" << std::endl;
        }
        // Add more breakpoints for Tetris-specific locations
        else if (registers.pc == 0x028D || registers.pc == 0x0290) {
            // Common entry points for Tetris VRAM initialization
            std::cout << "CPU TRACE: At VRAM init location: 0x" << std::hex << registers.pc 
                      << " AF=" << registers.af << " BC=" << registers.bc 
                      << " DE=" << registers.de << " HL=" << registers.hl << std::dec << std::endl;
        }
        // Add general instruction trace every 100,000 instructions
        else if (debug_instruction_count % 100000 == 0) {
            std::cout << "CPU Status: PC=0x" << std::hex << registers.pc 
                      << " Executed " << std::dec << debug_instruction_count << " instructions" 
                      << " Cycles=" << cycles << std::endl;
        }

        if (halt_bug_active) {
            // HALT bug: PC is not incremented for the first fetch after HALT
            halt_bug_active = false;
            current_opcode = memory.read(registers.pc);
            
            // But the next opcode will be fetched from PC+1
            // This is what makes it a "bug"
        } else {
            // Normal instruction fetch
            current_opcode = memory.read(registers.pc++);
        }

        debug_instruction_count++;
        
        // Get the instruction details from the opcode
        current_instruction = &instructions.get(current_opcode);
        
        // Fetch any instruction data needed based on addressing mode
        fetch_adress();
        
        // Execute the instruction
        execute();
    } else {
        // Consume a pending cycle
        pending_cycles--;
    }
    
    // Increment total cycle count
    cycles++;
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
    // printf("Executing: 0x%02X (%s) at PC: 0x%04X | ", 
    //        current_opcode, 
    //        Instructions::get_type_name(current_instruction->type).c_str(),
    //        registers.pc);
    
    // Print addressing mode and registers involved
    // printf("Mode: %d | ", static_cast<int>(current_instruction->addr_mode));
    
    if (current_instruction->reg1 != Instructions::RegType::NONE) {
        // printf("Reg1: %s ", Instructions::get_reg_name(current_instruction->reg1).c_str());
    }
    
    if (current_instruction->reg2 != Instructions::RegType::NONE) {
        // printf("Reg2: %s ", Instructions::get_reg_name(current_instruction->reg2).c_str());
    }
    
    // Print immediate value if applicable
    if (current_instruction->addr_mode == Instructions::AddrMode::R_D8 || 
        current_instruction->addr_mode == Instructions::AddrMode::D8 ||
        current_instruction->addr_mode == Instructions::AddrMode::MR_D8) {
        // printf("| Imm: 0x%02X ", current_instruction_data.immediate_value & 0xFF);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D16 || 
               current_instruction->addr_mode == Instructions::AddrMode::D16 ||
               current_instruction->addr_mode == Instructions::AddrMode::D16_R) {
        // printf("| Imm: 0x%04X ", current_instruction_data.immediate_value);
    }
    
    // Print register values
    // printf("\n  Registers: A:0x%02X F:0x%02X B:0x%02X C:0x%02X D:0x%02X E:0x%02X H:0x%02X L:0x%02X SP:0x%04X",
    //        registers.a, registers.f, registers.b, registers.c, 
    //        registers.d, registers.e, registers.h, registers.l, registers.sp);
    
    // Print flags
    // printf("\n  Flags: Z:%d N:%d H:%d C:%d", 
    //        getFlag(FLAG_Z) ? 1 : 0, 
    //        getFlag(FLAG_N) ? 1 : 0, 
    //        getFlag(FLAG_H) ? 1 : 0, 
    //        getFlag(FLAG_C) ? 1 : 0);
    
    // Print 16-bit register pairs
    // printf("\n  Pairs: AF:0x%04X BC:0x%04X DE:0x%04X HL:0x%04X\n", 
    //        registers.af, registers.bc, registers.de, registers.hl);
    
    //Debug section end
    bool branch_taken = false; // Track if branch instructions actually branch
    
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
            branch_taken = executeJP();
            break;
            
        case Instructions::Type::JR:
            // Jump relative
            branch_taken = executeJR();
            break;
            
        case Instructions::Type::CALL:
            // Call subroutine
            branch_taken = executeCALL();
            break;
            
        case Instructions::Type::RET:
            // Return from subroutine
            branch_taken = executeRET();
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
            // printf("Unimplemented instruction: %s\n", 
            //        Instructions::get_type_name(current_instruction->type).c_str());
            break;
    }
    
    // Update the cycle count based on the result of the instruction
    // For conditional instructions, this ensures we use the right cycle count
    pending_cycles = get_instruction_cycles(current_instruction, branch_taken) - 1;
}

// Helper to compute cycles for a given instruction
uint8_t CPU::get_instruction_cycles(const Instructions::Instruction* instr, bool branch_taken) {
    // For conditional instructions, use alt_cycles when branch is not taken
    if (instr->cond != Instructions::CondType::NONE) {
        return branch_taken ? instr->cycles : instr->alt_cycles;
    }
    return instr->cycles;
}

bool CPU::handleInterrupts() {
    // If IME is disabled, interrupts are not processed
    if (!ime) {
        return false;
    }
    
    // Read interrupt flags (IF) and interrupt enable register (IE)
    uint8_t if_reg = memory.read(0xFF0F);
    uint8_t ie_reg = memory.read(0xFFFF);
    
    // Mask enabled interrupts with requested interrupts
    uint8_t active_interrupts = if_reg & ie_reg & 0x1F;
    
    // If no interrupts are active, return false
    if (active_interrupts == 0) {
        return false;
    }
    
    // Exit HALT mode if any interrupt is requested (even if disabled)
    halted = false;
    
    // Process each interrupt in priority order
    // Priority: VBlank (0) > LCD STAT (1) > Timer (2) > Serial (3) > Joypad (4)
    
    // Process VBlank interrupt (bit 0)
    if (active_interrupts & 0x01) {
        ime = false;  // Disable interrupts while handling one
        
        // Clear VBlank interrupt flag
        if_reg &= ~0x01;
        memory.write(0xFF0F, if_reg);
        
        // Push PC to stack
        memory.write(--registers.sp, registers.pc >> 8);
        memory.write(--registers.sp, registers.pc & 0xFF);
        
        // Jump to interrupt handler
        registers.pc = 0x0040;  // VBlank handler address
        
        cycles += 12;  // Interrupt takes 12 cycles
        return true;
    }
    
    // Process LCD STAT interrupt (bit 1)
    if (active_interrupts & 0x02) {
        ime = false;
        
        // Clear LCD STAT interrupt flag
        if_reg &= ~0x02;
        memory.write(0xFF0F, if_reg);
        
        // Push PC to stack
        memory.write(--registers.sp, registers.pc >> 8);
        memory.write(--registers.sp, registers.pc & 0xFF);
        
        // Jump to interrupt handler
        registers.pc = 0x0048;  // LCD STAT handler address
        
        cycles += 12;
        return true;
    }
    
    // Process Timer interrupt (bit 2)
    if (active_interrupts & 0x04) {
        ime = false;
        
        // Clear Timer interrupt flag
        if_reg &= ~0x04;
        memory.write(0xFF0F, if_reg);
        
        // Push PC to stack
        memory.write(--registers.sp, registers.pc >> 8);
        memory.write(--registers.sp, registers.pc & 0xFF);
        
        // Jump to interrupt handler
        registers.pc = 0x0050;  // Timer handler address
        
        cycles += 12;
        return true;
    }
    
    // Process Serial interrupt (bit 3)
    if (active_interrupts & 0x08) {
        ime = false;
        
        // Clear Serial interrupt flag
        if_reg &= ~0x08;
        memory.write(0xFF0F, if_reg);
        
        // Push PC to stack
        memory.write(--registers.sp, registers.pc >> 8);
        memory.write(--registers.sp, registers.pc & 0xFF);
        
        // Jump to interrupt handler
        registers.pc = 0x0058;  // Serial handler address
        
        cycles += 12;
        return true;
    }
    
    // Process Joypad interrupt (bit 4)
    if (active_interrupts & 0x10) {
        ime = false;
        
        // Clear Joypad interrupt flag
        if_reg &= ~0x10;
        memory.write(0xFF0F, if_reg);
        
        // Push PC to stack
        memory.write(--registers.sp, registers.pc >> 8);
        memory.write(--registers.sp, registers.pc & 0xFF);
        
        // Jump to interrupt handler
        registers.pc = 0x0060;  // Joypad handler address
        
        cycles += 12;
        return true;
    }
    
    return false;
}

// Update the reset method to initialize registers correctly
void CPU::reset() {
    // Initialize registers to post-boot values for DMG
    registers.af = 0x01B0;  // A=0x01, F=0xB0 (Z flag set)
    registers.bc = 0x0013;  // B=0x00, C=0x13
    registers.de = 0x00D8;  // D=0x00, E=0xD8
    registers.hl = 0x014D;  // H=0x01, L=0x4D
    registers.pc = 0x0100;  // PC=0x0100 (entry point)
    registers.sp = 0xFFFE;  // SP=0xFFFE
    
    // Enable interrupts by default
    ime = true;
    halted = false;
    stopped = false;
    
    // Reset debug counter
    debug_instruction_count = 0;
}
