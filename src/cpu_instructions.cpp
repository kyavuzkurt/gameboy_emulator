#include "cpu.hpp"
#include "instructions.hpp"
#include "memory.hpp"
#include <stdio.h>

// Helper methods for instruction execution
void CPU::executeLD() {
    // Load instruction - handles different addressing modes
    switch (current_instruction->addr_mode) {
        case Instructions::AddrMode::R_R: {
            // Register to register
            uint8_t value = getRegister8Bit(current_instruction->reg2);
            setRegister8Bit(current_instruction->reg1, value);
            break;
        }
        
        case Instructions::AddrMode::R_D8: {
            // Load immediate 8-bit value into register
            setRegister8Bit(current_instruction->reg1, current_instruction_data.immediate_value & 0xFF);
            break;
        }
        
        case Instructions::AddrMode::R_D16: {
            // Load immediate 16-bit value into register pair
            setRegister16Bit(current_instruction->reg1, current_instruction_data.immediate_value);
            break;
        }
        
        case Instructions::AddrMode::R_MR: {
            // Load from memory address in register to register
            uint16_t addr = getRegister16Bit(current_instruction->reg2);
            uint8_t value = memory.read(addr);
            setRegister8Bit(current_instruction->reg1, value);
            break;
        }
        
        case Instructions::AddrMode::MR_R: {
            // Load from register to memory address in register
            uint16_t addr = getRegister16Bit(current_instruction->reg1);
            uint8_t value = getRegister8Bit(current_instruction->reg2);
            memory.write(addr, value);
            break;
        }
        
        case Instructions::AddrMode::R_HLI: {
            // Load from memory at HL to register, then increment HL
            uint16_t addr = registers.hl;
            uint8_t value = memory.read(addr);
            setRegister8Bit(current_instruction->reg1, value);
            registers.hl++;
            break;
        }
        
        case Instructions::AddrMode::R_HLD: {
            // Load from memory at HL to register, then decrement HL
            uint16_t addr = registers.hl;
            uint8_t value = memory.read(addr);
            setRegister8Bit(current_instruction->reg1, value);
            registers.hl--;
            break;
        }
        
        case Instructions::AddrMode::HLI_R: {
            // Load from register to memory at HL, then increment HL
            uint16_t addr = registers.hl;
            uint8_t value = getRegister8Bit(current_instruction->reg2);
            memory.write(addr, value);
            registers.hl++;
            break;
        }
        
        case Instructions::AddrMode::HLD_R: {
            // Load from register to memory at HL, then decrement HL
            uint16_t addr = registers.hl;
            uint8_t value = getRegister8Bit(current_instruction->reg2);
            memory.write(addr, value);
            registers.hl--;
            break;
        }
        
        case Instructions::AddrMode::R_A8: {
            // Load from high RAM (0xFF00 + immediate 8-bit) to register
            uint16_t addr = 0xFF00 + (current_instruction_data.immediate_value & 0xFF);
            uint8_t value = memory.read(addr);
            setRegister8Bit(current_instruction->reg1, value);
            break;
        }
        
        case Instructions::AddrMode::A8_R: {
            // Load from register to high RAM (0xFF00 + immediate 8-bit)
            uint16_t addr = 0xFF00 + (current_instruction_data.immediate_value & 0xFF);
            uint8_t value = getRegister8Bit(current_instruction->reg2);
            memory.write(addr, value);
            break;
        }
        
        case Instructions::AddrMode::R_A16: {
            // Load from absolute 16-bit address to register
            uint16_t addr = current_instruction_data.immediate_value;
            uint8_t value = memory.read(addr);
            setRegister8Bit(current_instruction->reg1, value);
            break;
        }
        
        case Instructions::AddrMode::A16_R: {
            // Load from register to absolute 16-bit address
            uint16_t addr = current_instruction_data.immediate_value;
            uint8_t value = getRegister8Bit(current_instruction->reg2);
            memory.write(addr, value);
            break;
        }
        
        case Instructions::AddrMode::HL_SPR: {
            // LD HL,SP+r8 (add signed immediate to SP and store in HL)
            int8_t offset = static_cast<int8_t>(current_instruction_data.immediate_value & 0xFF);
            uint16_t sp = registers.sp;
            
            // Calculate flags before the addition
            // Half-carry occurs if bit 3 carries into bit 4
            setFlag(FLAG_H, ((sp & 0xF) + (offset & 0xF)) > 0xF);
            // Carry occurs if bit 7 carries into bit 8
            setFlag(FLAG_C, ((sp & 0xFF) + (offset & 0xFF)) > 0xFF);
            
            // Clear N and Z flags
            setFlag(FLAG_N, false);
            setFlag(FLAG_Z, false);
            
            // Perform the addition and store in HL
            registers.hl = sp + offset;
            break;
        }
        
        case Instructions::AddrMode::D16_R: {
            // Store SP at immediate 16-bit address
            if (current_instruction->reg2 == Instructions::RegType::SP) {
                uint16_t addr = current_instruction_data.immediate_value;
                memory.write16(addr, registers.sp);
            }
            break;
        }
        
        default:
            printf("Unimplemented LD addressing mode: %d\n", 
                   static_cast<int>(current_instruction->addr_mode));
            break;
    }
}

void CPU::executeINC() {
    // Increment register or memory
    if (isRegister16Bit(current_instruction->reg1)) {
        // 16-bit register increment
        uint16_t value = getRegister16Bit(current_instruction->reg1);
        value++;
        setRegister16Bit(current_instruction->reg1, value);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Memory increment
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        uint8_t value = memory.read(addr);
        
        // Set flags (Z, N, H) - 8-bit INC affects flags
        setFlag(FLAG_N, false);
        setFlag(FLAG_H, (value & 0x0F) == 0x0F); // Half carry if lower nibble is 0xF
        
        value++;
        memory.write(addr, value);
        
        setFlag(FLAG_Z, value == 0);
    } else {
        // 8-bit register increment
        uint8_t value = getRegister8Bit(current_instruction->reg1);
        
        // Set flags (Z, N, H) - 8-bit INC affects flags
        setFlag(FLAG_N, false);
        setFlag(FLAG_H, (value & 0x0F) == 0x0F); // Half carry if lower nibble is 0xF
        
        value++;
        setRegister8Bit(current_instruction->reg1, value);
        
        setFlag(FLAG_Z, value == 0);
    }
}

void CPU::executeDEC() {
    // Decrement register or memory
    if (isRegister16Bit(current_instruction->reg1)) {
        // 16-bit register decrement
        uint16_t value = getRegister16Bit(current_instruction->reg1);
        value--;
        setRegister16Bit(current_instruction->reg1, value);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Memory decrement
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        uint8_t value = memory.read(addr);
        
        // Set flags (Z, N, H) - 8-bit DEC affects flags
        setFlag(FLAG_N, true);
        setFlag(FLAG_H, (value & 0x0F) == 0x00); // Half carry if lower nibble is 0
        
        value--;
        memory.write(addr, value);
        
        setFlag(FLAG_Z, value == 0);
    } else {
        // 8-bit register decrement
        uint8_t value = getRegister8Bit(current_instruction->reg1);
        
        // Set flags (Z, N, H) - 8-bit DEC affects flags
        setFlag(FLAG_N, true);
        setFlag(FLAG_H, (value & 0x0F) == 0x00); // Half carry if lower nibble is 0
        
        value--;
        setRegister8Bit(current_instruction->reg1, value);
        
        setFlag(FLAG_Z, value == 0);
    }
}

void CPU::executeADD() {
    // Addition
    if (current_instruction->reg1 == Instructions::RegType::HL && 
        isRegister16Bit(current_instruction->reg2)) {
        // 16-bit addition (HL += reg16)
        uint16_t hl = registers.hl;
        uint16_t value = getRegister16Bit(current_instruction->reg2);
        
        // Set flags
        setFlag(FLAG_N, false);
        setFlag(FLAG_H, ((hl & 0x0FFF) + (value & 0x0FFF)) > 0x0FFF); // Half carry
        setFlag(FLAG_C, (uint32_t)hl + (uint32_t)value > 0xFFFF); // Carry
        
        registers.hl = hl + value;
    } else if (current_instruction->reg1 == Instructions::RegType::A) {
        // 8-bit addition (A += reg8 or memory)
        uint8_t a = registers.a;
        uint8_t value;
        
        if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
            // Add from memory
            uint16_t addr = getRegister16Bit(current_instruction->reg2);
            value = memory.read(addr);
        } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
            // Add immediate value
            value = current_instruction_data.immediate_value & 0xFF;
        } else {
            // Add from register
            value = getRegister8Bit(current_instruction->reg2);
        }
        
        // Set flags
        setFlag(FLAG_N, false);
        setFlag(FLAG_H, ((a & 0x0F) + (value & 0x0F)) > 0x0F); // Half carry
        setFlag(FLAG_C, (uint16_t)a + (uint16_t)value > 0xFF); // Carry
        
        registers.a = a + value;
        setFlag(FLAG_Z, registers.a == 0);
    }
}

void CPU::executeSUB() {
    // Subtraction
    uint8_t a = registers.a;
    uint8_t value;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // Subtract from memory
        uint16_t addr = getRegister16Bit(current_instruction->reg2);
        value = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // Subtract immediate value
        value = current_instruction_data.immediate_value & 0xFF;
    } else {
        // Subtract from register
        value = getRegister8Bit(current_instruction->reg2);
    }
    
    // Set flags
    setFlag(FLAG_N, true);  // Subtraction operation
    setFlag(FLAG_H, (a & 0x0F) < (value & 0x0F));  // Half carry
    setFlag(FLAG_C, a < value);  // Carry
    
    registers.a = a - value;
    setFlag(FLAG_Z, registers.a == 0);  // Zero flag
}

void CPU::executeAND() {
    // Logical AND with accumulator
    uint8_t value;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // AND with memory value
        uint16_t addr = getRegister16Bit(current_instruction->reg2);
        value = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // AND with immediate value
        value = current_instruction_data.immediate_value & 0xFF;
    } else {
        // AND with register
        value = getRegister8Bit(current_instruction->reg2);
    }
    
    // Perform AND operation
    registers.a &= value;
    
    // Set flags
    setFlag(FLAG_Z, registers.a == 0);  // Zero flag
    setFlag(FLAG_N, false);  // Not subtraction
    setFlag(FLAG_H, true);   // Half carry is always set
    setFlag(FLAG_C, false);  // Carry is always reset
}

void CPU::executeOR() {
    // Logical OR with accumulator
    uint8_t value;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // OR with memory value
        uint16_t addr = getRegister16Bit(current_instruction->reg2);
        value = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // OR with immediate value
        value = current_instruction_data.immediate_value & 0xFF;
    } else {
        // OR with register
        value = getRegister8Bit(current_instruction->reg2);
    }
    
    // Perform OR operation
    registers.a |= value;
    
    // Set flags
    setFlag(FLAG_Z, registers.a == 0);  // Zero flag
    setFlag(FLAG_N, false);  // Not subtraction
    setFlag(FLAG_H, false);  // Half carry is always reset
    setFlag(FLAG_C, false);  // Carry is always reset
}

void CPU::executeXOR() {
    // Logical XOR with accumulator
    uint8_t value;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // XOR with memory value
        uint16_t addr = getRegister16Bit(current_instruction->reg2);
        value = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // XOR with immediate value
        value = current_instruction_data.immediate_value & 0xFF;
    } else {
        // XOR with register
        value = getRegister8Bit(current_instruction->reg2);
    }
    
    // Perform XOR operation
    registers.a ^= value;
    
    // Set flags
    setFlag(FLAG_Z, registers.a == 0);  // Zero flag
    setFlag(FLAG_N, false);  // Not subtraction
    setFlag(FLAG_H, false);  // Half carry is always reset
    setFlag(FLAG_C, false);  // Carry is always reset
}

void CPU::executeJP() {
    // Jump to address
    bool shouldJump = true;
    
    // Check condition if it's a conditional jump
    if (current_instruction->addr_mode == Instructions::AddrMode::R_D16) {
        // For conditional jumps, we'll use reg1 to determine the condition
        switch (current_instruction->reg1) {
            case Instructions::RegType::CC_NZ:
                shouldJump = !getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_Z:
                shouldJump = getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_NC:
                shouldJump = !getFlag(FLAG_C);
                break;
            case Instructions::RegType::CC_C:
                shouldJump = getFlag(FLAG_C);
                break;
            default:
                // Unconditional jump
                break;
        }
    }
    
    if (shouldJump) {
        if (current_instruction->addr_mode == Instructions::AddrMode::R) {
            // Jump to address in register (HL)
            registers.pc = registers.hl;
        } else {
            // Jump to immediate address
            registers.pc = current_instruction_data.immediate_value;
        }
    }
}

void CPU::executeJR() {
    // Jump relative (PC += signed immediate)
    bool shouldJump = true;
    
    // Check condition if it's a conditional jump
    if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // For conditional jumps, we'll use reg1 to determine the condition
        switch (current_instruction->reg1) {
            case Instructions::RegType::CC_NZ:
                shouldJump = !getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_Z:
                shouldJump = getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_NC:
                shouldJump = !getFlag(FLAG_C);
                break;
            case Instructions::RegType::CC_C:
                shouldJump = getFlag(FLAG_C);
                break;
            default:
                // Unconditional jump
                break;
        }
    }
    
    if (shouldJump) {
        // Convert to signed 8-bit offset
        int8_t offset = static_cast<int8_t>(current_instruction_data.immediate_value & 0xFF);
        registers.pc += offset;
    }
}

void CPU::executeCALL() {
    // Call subroutine
    bool shouldCall = true;
    
    // Check condition if it's a conditional call
    if (current_instruction->addr_mode == Instructions::AddrMode::R_D16) {
        // For conditional calls, we'll use reg1 to determine the condition
        switch (current_instruction->reg1) {
            case Instructions::RegType::CC_NZ:
                shouldCall = !getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_Z:
                shouldCall = getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_NC:
                shouldCall = !getFlag(FLAG_C);
                break;
            case Instructions::RegType::CC_C:
                shouldCall = getFlag(FLAG_C);
                break;
            default:
                // Unconditional call
                break;
        }
    }
    
    if (shouldCall) {
        // Push current PC to stack
        registers.sp -= 2;
        // Use the write method twice instead of write16
        memory.write(registers.sp, registers.pc & 0xFF);
        memory.write(registers.sp + 1, registers.pc >> 8);
        
        // Jump to call address
        registers.pc = current_instruction_data.immediate_value;
    }
}

void CPU::executeRET() {
    // Return from subroutine
    bool shouldReturn = true;
    
    // Check condition if it's a conditional return
    if (current_instruction->addr_mode == Instructions::AddrMode::IMP) {
        // For conditional returns, we'll use reg1 to determine the condition
        switch (current_instruction->reg1) {
            case Instructions::RegType::CC_NZ:
                shouldReturn = !getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_Z:
                shouldReturn = getFlag(FLAG_Z);
                break;
            case Instructions::RegType::CC_NC:
                shouldReturn = !getFlag(FLAG_C);
                break;
            case Instructions::RegType::CC_C:
                shouldReturn = getFlag(FLAG_C);
                break;
            default:
                // Unconditional return
                break;
        }
    }
    
    if (shouldReturn) {
        // Pop return address from stack
        // Use read method twice instead of read16
        uint16_t low_byte = memory.read(registers.sp);
        uint16_t high_byte = memory.read(registers.sp + 1);
        registers.pc = (high_byte << 8) | low_byte;
        registers.sp += 2;
    }
}

void CPU::executePUSH() {
    // Push register pair to stack
    uint16_t value = getRegister16Bit(current_instruction->reg1);
    
    // Decrement stack pointer and push value
    registers.sp -= 2;
    // Use write method twice instead of write16
    memory.write(registers.sp, value & 0xFF);
    memory.write(registers.sp + 1, value >> 8);
}

void CPU::executePOP() {
    // Pop value from stack to register pair
    // Use read method twice instead of read16
    uint16_t low_byte = memory.read(registers.sp);
    uint16_t high_byte = memory.read(registers.sp + 1);
    uint16_t value = (high_byte << 8) | low_byte;
    registers.sp += 2;
    
    // Special case for AF: lower 4 bits of F are always 0
    if (current_instruction->reg1 == Instructions::RegType::AF) {
        value &= 0xFFF0;
    }
    
    setRegister16Bit(current_instruction->reg1, value);
}

void CPU::executeRLCA() {
    // Rotate Left Circular Accumulator
    uint8_t a = registers.a;
    uint8_t bit7 = (a & 0x80) >> 7;  // Get the highest bit
    
    // Rotate left
    registers.a = (a << 1) | bit7;
    
    // Set flags
    setFlag(FLAG_Z, false);  // Z is always reset
    setFlag(FLAG_N, false);  // N is always reset
    setFlag(FLAG_H, false);  // H is always reset
    setFlag(FLAG_C, bit7);   // C gets the old bit 7
}

void CPU::executeRRCA() {
    // Rotate Right Circular Accumulator
    uint8_t a = registers.a;
    uint8_t bit0 = a & 0x01;  // Get the lowest bit
    
    // Rotate right
    registers.a = (a >> 1) | (bit0 << 7);
    
    // Set flags
    setFlag(FLAG_Z, false);  // Z is always reset
    setFlag(FLAG_N, false);  // N is always reset
    setFlag(FLAG_H, false);  // H is always reset
    setFlag(FLAG_C, bit0);   // C gets the old bit 0
}

void CPU::executeRLA() {
    // Rotate Left Accumulator (through carry)
    uint8_t a = registers.a;
    uint8_t bit7 = (a & 0x80) >> 7;  // Get the highest bit
    uint8_t oldCarry = getFlag(FLAG_C) ? 1 : 0;
    
    // Rotate left through carry
    registers.a = (a << 1) | oldCarry;
    
    // Set flags
    setFlag(FLAG_Z, false);  // Z is always reset
    setFlag(FLAG_N, false);  // N is always reset
    setFlag(FLAG_H, false);  // H is always reset
    setFlag(FLAG_C, bit7);   // C gets the old bit 7
}

void CPU::executeRRA() {
    // Rotate Right Accumulator (through carry)
    uint8_t a = registers.a;
    uint8_t bit0 = a & 0x01;  // Get the lowest bit
    uint8_t oldCarry = getFlag(FLAG_C) ? 1 : 0;
    
    // Rotate right through carry
    registers.a = (a >> 1) | (oldCarry << 7);
    
    // Set flags
    setFlag(FLAG_Z, false);  // Z is always reset
    setFlag(FLAG_N, false);  // N is always reset
    setFlag(FLAG_H, false);  // H is always reset
    setFlag(FLAG_C, bit0);   // C gets the old bit 0
}

void CPU::executeDAA() {
    // Decimal Adjust Accumulator
    // Adjusts A to a BCD number after BCD operations
    uint8_t a = registers.a;
    uint8_t correction = 0;
    
    if (getFlag(FLAG_H) || (!getFlag(FLAG_N) && (a & 0x0F) > 9)) {
        correction |= 0x06;
    }
    
    if (getFlag(FLAG_C) || (!getFlag(FLAG_N) && a > 0x99)) {
        correction |= 0x60;
        setFlag(FLAG_C, true);
    }
    
    // Add or subtract the correction value
    if (getFlag(FLAG_N)) {
        a -= correction;
    } else {
        a += correction;
    }
    
    registers.a = a;
    
    // Update flags
    setFlag(FLAG_Z, a == 0);
    setFlag(FLAG_H, false);
}

void CPU::executeCPL() {
    // Complement (NOT) on register A
    registers.a = ~registers.a;
    
    // Set flags
    setFlag(FLAG_N, true);
    setFlag(FLAG_H, true);
    // Z and C flags are unaffected
}

void CPU::executeSCF() {
    // Set Carry Flag
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, true);
    // Z flag is unaffected
}

void CPU::executeCCF() {
    // Complement Carry Flag
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, !getFlag(FLAG_C));
    // Z flag is unaffected
}

void CPU::executeHALT() {
    // Halt the CPU until an interrupt occurs
    halted = true;
}

void CPU::executeADC() {
    // Add with Carry
    uint8_t a = registers.a;
    uint8_t value;
    uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // Add from memory
        uint16_t addr = getRegister16Bit(current_instruction->reg2);
        value = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // Add immediate value
        value = current_instruction_data.immediate_value & 0xFF;
    } else {
        // Add from register
        value = getRegister8Bit(current_instruction->reg2);
    }
    
    // Calculate result with carry
    uint16_t result = a + value + carry;
    
    // Set flags
    setFlag(FLAG_Z, (result & 0xFF) == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, ((a & 0x0F) + (value & 0x0F) + carry) > 0x0F); // Half carry
    setFlag(FLAG_C, result > 0xFF); // Carry
    
    registers.a = result & 0xFF;
}

void CPU::executeSBC() {
    // Subtract with Carry
    uint8_t a = registers.a;
    uint8_t value;
    uint8_t carry = getFlag(FLAG_C) ? 1 : 0;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // Subtract from memory
        uint16_t addr = getRegister16Bit(current_instruction->reg2);
        value = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // Subtract immediate value
        value = current_instruction_data.immediate_value & 0xFF;
    } else {
        // Subtract from register
        value = getRegister8Bit(current_instruction->reg2);
    }
    
    // Calculate result with carry
    int result = a - value - carry;
    
    // Set flags
    setFlag(FLAG_Z, (result & 0xFF) == 0);
    setFlag(FLAG_N, true);
    setFlag(FLAG_H, ((int)(a & 0x0F) - (int)(value & 0x0F) - (int)carry) < 0); // Half borrow
    setFlag(FLAG_C, result < 0); // Borrow
    
    registers.a = result & 0xFF;
}

void CPU::executeCP() {
    // Compare (subtract without storing result)
    uint8_t a = registers.a;
    uint8_t value;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // Compare with memory
        uint16_t addr = getRegister16Bit(current_instruction->reg2);
        value = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_D8) {
        // Compare with immediate value
        value = current_instruction_data.immediate_value & 0xFF;
    } else {
        // Compare with register
        value = getRegister8Bit(current_instruction->reg2);
    }
    
    // Calculate result (don't store it)
    int result = a - value;
    
    // Set flags
    setFlag(FLAG_Z, (result & 0xFF) == 0);
    setFlag(FLAG_N, true);
    setFlag(FLAG_H, ((int)(a & 0x0F) - (int)(value & 0x0F)) < 0); // Half borrow
    setFlag(FLAG_C, result < 0); // Borrow
}

void CPU::executeRETI() {
    // Return from interrupt
    executeRET(); // Perform normal return
    ime = true;   // Enable interrupts
}

void CPU::executeLDH() {
    // Load to/from high RAM area (0xFF00 + offset)
    if (current_instruction->addr_mode == Instructions::AddrMode::A8_R) {
        // LDH (a8),A - Store A in high RAM
        uint16_t addr = 0xFF00 + (current_instruction_data.immediate_value & 0xFF);
        memory.write(addr, registers.a);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_A8) {
        // LDH A,(a8) - Load A from high RAM
        uint16_t addr = 0xFF00 + (current_instruction_data.immediate_value & 0xFF);
        registers.a = memory.read(addr);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::MR_R) {
        // LD (C),A - Store A in high RAM at 0xFF00+C
        uint16_t addr = 0xFF00 + registers.c;
        memory.write(addr, registers.a);
    } else if (current_instruction->addr_mode == Instructions::AddrMode::R_MR) {
        // LD A,(C) - Load A from high RAM at 0xFF00+C
        uint16_t addr = 0xFF00 + registers.c;
        registers.a = memory.read(addr);
    }
}

void CPU::executeDI() {
    // Disable Interrupts
    ime = false;
}

void CPU::executeEI() {
    // Enable Interrupts
    ime = true;
}

void CPU::executeRST() {
    // Reset - Call to predefined address
    uint16_t addr = current_instruction->param * 8; // RST param is 0-7, address is param*8
    
    // Push current PC to stack
    registers.sp -= 2;
    memory.write16(registers.sp, registers.pc);
    
    // Jump to reset address
    registers.pc = addr;
}

void CPU::executeRLC() {
    // Rotate Left Circular
    uint8_t value;
    uint8_t bit7;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        bit7 = (value & 0x80) >> 7;
        value = (value << 1) | bit7;
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        bit7 = (value & 0x80) >> 7;
        value = (value << 1) | bit7;
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, bit7);
}

void CPU::executeRRC() {
    // Rotate Right Circular
    uint8_t value;
    uint8_t bit0;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        bit0 = value & 0x01;
        value = (value >> 1) | (bit0 << 7);
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        bit0 = value & 0x01;
        value = (value >> 1) | (bit0 << 7);
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, bit0);
}

void CPU::executeRL() {
    // Rotate Left through carry
    uint8_t value;
    uint8_t bit7;
    uint8_t oldCarry = getFlag(FLAG_C) ? 1 : 0;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        bit7 = (value & 0x80) >> 7;
        value = (value << 1) | oldCarry;
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        bit7 = (value & 0x80) >> 7;
        value = (value << 1) | oldCarry;
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, bit7);
}

void CPU::executeRR() {
    // Rotate Right through carry
    uint8_t value;
    uint8_t bit0;
    uint8_t oldCarry = getFlag(FLAG_C) ? 1 : 0;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        bit0 = value & 0x01;
        value = (value >> 1) | (oldCarry << 7);
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        bit0 = value & 0x01;
        value = (value >> 1) | (oldCarry << 7);
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, bit0);
}

void CPU::executeSLA() {
    // Shift Left Arithmetic
    uint8_t value;
    uint8_t bit7;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        bit7 = (value & 0x80) >> 7;
        value = value << 1;
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        bit7 = (value & 0x80) >> 7;
        value = value << 1;
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, bit7);
}

void CPU::executeSRA() {
    // Shift Right Arithmetic (MSB doesn't change)
    uint8_t value;
    uint8_t bit0;
    uint8_t bit7;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        bit0 = value & 0x01;
        bit7 = value & 0x80;
        value = (value >> 1) | bit7;
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        bit0 = value & 0x01;
        bit7 = value & 0x80;
        value = (value >> 1) | bit7;
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, bit0);
}

void CPU::executeSWAP() {
    // Swap upper and lower nibbles
    uint8_t value;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        value = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        value = ((value & 0x0F) << 4) | ((value & 0xF0) >> 4);
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, false);
}

void CPU::executeSRL() {
    // Shift Right Logical (MSB becomes 0)
    uint8_t value;
    uint8_t bit0;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Operate on memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        bit0 = value & 0x01;
        value = value >> 1;
        memory.write(addr, value);
    } else {
        // Operate on register
        value = getRegister8Bit(current_instruction->reg1);
        bit0 = value & 0x01;
        value = value >> 1;
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // Set flags
    setFlag(FLAG_Z, value == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, false);
    setFlag(FLAG_C, bit0);
}

void CPU::executeBIT() {
    // Test bit in register or memory
    uint8_t value;
    uint8_t bitPos = current_instruction->param;
    uint8_t bitMask = 1 << bitPos;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Test bit in memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
    } else {
        // Test bit in register
        value = getRegister8Bit(current_instruction->reg1);
    }
    
    // Set flags
    setFlag(FLAG_Z, (value & bitMask) == 0);
    setFlag(FLAG_N, false);
    setFlag(FLAG_H, true);
    // C flag is unaffected
}

void CPU::executeRES() {
    // Reset bit in register or memory
    uint8_t value;
    uint8_t bitPos = current_instruction->param;
    uint8_t bitMask = ~(1 << bitPos);
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Reset bit in memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        value &= bitMask;
        memory.write(addr, value);
    } else {
        // Reset bit in register
        value = getRegister8Bit(current_instruction->reg1);
        value &= bitMask;
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // No flags are affected
}

void CPU::executeSET() {
    // Set bit in register or memory
    uint8_t value;
    uint8_t bitPos = current_instruction->param;
    uint8_t bitMask = 1 << bitPos;
    
    if (current_instruction->addr_mode == Instructions::AddrMode::MR) {
        // Set bit in memory
        uint16_t addr = getRegister16Bit(current_instruction->reg1);
        value = memory.read(addr);
        value |= bitMask;
        memory.write(addr, value);
    } else {
        // Set bit in register
        value = getRegister8Bit(current_instruction->reg1);
        value |= bitMask;
        setRegister8Bit(current_instruction->reg1, value);
    }
    
    // No flags are affected
} 