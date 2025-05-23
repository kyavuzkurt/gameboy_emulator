#include <instructions.hpp>

Instructions::Instructions() {
    initialize_instructions();
    initialize_cb_instructions();
}

const Instructions::Instruction& Instructions::get(uint8_t opcode) const {
    return instructions[opcode];
}

const Instructions::Instruction& Instructions::getCB(uint8_t opcode) const {
    return cb_instructions[opcode];
}

std::string Instructions::get_type_name(Type type) {
    switch (type) {
        case Type::NONE: return "NONE";
        case Type::NOP: return "NOP";
        case Type::LD: return "LD";
        case Type::INC: return "INC";
        case Type::DEC: return "DEC";
        case Type::RLCA: return "RLCA";
        case Type::ADD: return "ADD";
        case Type::RRCA: return "RRCA";
        case Type::STOP: return "STOP";
        case Type::RLA: return "RLA";
        case Type::JR: return "JR";
        case Type::RRA: return "RRA";
        case Type::DAA: return "DAA";
        case Type::CPL: return "CPL";
        case Type::SCF: return "SCF";
        case Type::CCF: return "CCF";
        case Type::HALT: return "HALT";
        case Type::ADC: return "ADC";
        case Type::SUB: return "SUB";
        case Type::SBC: return "SBC";
        case Type::AND: return "AND";
        case Type::XOR: return "XOR";
        case Type::OR: return "OR";
        case Type::CP: return "CP";
        case Type::POP: return "POP";
        case Type::JP: return "JP";
        case Type::PUSH: return "PUSH";
        case Type::RET: return "RET";
        case Type::CB: return "CB";
        case Type::CALL: return "CALL";
        case Type::RETI: return "RETI";
        case Type::LDH: return "LDH";
        case Type::DI: return "DI";
        case Type::EI: return "EI";
        case Type::RST: return "RST";
        case Type::ERR: return "ERR";
        // CB instructions
        case Type::RLC: return "RLC";
        case Type::RRC: return "RRC";
        case Type::RL: return "RL";
        case Type::RR: return "RR";
        case Type::SLA: return "SLA";
        case Type::SRA: return "SRA";
        case Type::SWAP: return "SWAP";
        case Type::SRL: return "SRL";
        case Type::BIT: return "BIT";
        case Type::RES: return "RES";
        case Type::SET: return "SET";
        default: return "UNKNOWN";
    }
}

std::string Instructions::get_reg_name(RegType reg) {
    switch (reg) {
        case RegType::NONE: return "NONE";
        case RegType::A: return "A";
        case RegType::F: return "F";
        case RegType::B: return "B";
        case RegType::C: return "C";
        case RegType::D: return "D";
        case RegType::E: return "E";
        case RegType::H: return "H";
        case RegType::L: return "L";
        case RegType::AF: return "AF";
        case RegType::BC: return "BC";
        case RegType::DE: return "DE";
        case RegType::HL: return "HL";
        case RegType::SP: return "SP";
        case RegType::PC: return "PC";
        default: return "UNKNOWN";
    }
}

uint8_t Instructions::get_reg_size(RegType reg) {
    switch (reg) {
        case RegType::AF:
        case RegType::BC:
        case RegType::DE:
        case RegType::HL:
        case RegType::SP:
        case RegType::PC:
            return 16;
        default:
            return 8;
    }
}

void Instructions::initialize_instructions() {
    // Initialize all instructions to NONE
    instructions.fill(Instruction());
    
    // 0x00 - 0x0F
    // Format: Type, AddrMode, Reg1, Reg2, CondType, Param, Cycles, Alt_Cycles
    instructions[0x00] = Instruction(Type::NOP, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x01] = Instruction(Type::LD, AddrMode::R_D16, RegType::BC, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x02] = Instruction(Type::LD, AddrMode::MR_R, RegType::BC, RegType::A, CondType::NONE, 0, 8);
    instructions[0x03] = Instruction(Type::INC, AddrMode::R, RegType::BC, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x04] = Instruction(Type::INC, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x05] = Instruction(Type::DEC, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x06] = Instruction(Type::LD, AddrMode::R_D8, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x07] = Instruction(Type::RLCA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x08] = Instruction(Type::LD, AddrMode::D16_R, RegType::NONE, RegType::SP, CondType::NONE, 0, 20);
    instructions[0x09] = Instruction(Type::ADD, AddrMode::R_R, RegType::HL, RegType::BC, CondType::NONE, 0, 8);
    instructions[0x0A] = Instruction(Type::LD, AddrMode::R_MR, RegType::A, RegType::BC, CondType::NONE, 0, 8);
    instructions[0x0B] = Instruction(Type::DEC, AddrMode::R, RegType::BC, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x0C] = Instruction(Type::INC, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x0D] = Instruction(Type::DEC, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x0E] = Instruction(Type::LD, AddrMode::R_D8, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x0F] = Instruction(Type::RRCA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);

    // 0x10 - 0x1F
    instructions[0x10] = Instruction(Type::STOP, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x11] = Instruction(Type::LD, AddrMode::R_D16, RegType::DE, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x12] = Instruction(Type::LD, AddrMode::MR_R, RegType::DE, RegType::A, CondType::NONE, 0, 8);
    instructions[0x13] = Instruction(Type::INC, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x14] = Instruction(Type::INC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x15] = Instruction(Type::DEC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x16] = Instruction(Type::LD, AddrMode::R_D8, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x17] = Instruction(Type::RLA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x18] = Instruction(Type::JR, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x19] = Instruction(Type::ADD, AddrMode::R_R, RegType::HL, RegType::DE, CondType::NONE, 0, 8);
    instructions[0x1A] = Instruction(Type::LD, AddrMode::R_MR, RegType::A, RegType::DE, CondType::NONE, 0, 8);
    instructions[0x1B] = Instruction(Type::DEC, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x1C] = Instruction(Type::INC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x1D] = Instruction(Type::DEC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x1E] = Instruction(Type::LD, AddrMode::R_D8, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x1F] = Instruction(Type::RRA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);

    // 0x20 - 0x2F
    instructions[0x20] = Instruction(Type::JR, AddrMode::CC_D8, RegType::CC_NZ, RegType::NONE, CondType::NZ, 0, 12, 8);  // Branch taken: 12, not taken: 8
    instructions[0x21] = Instruction(Type::LD, AddrMode::R_D16, RegType::HL, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x22] = Instruction(Type::LD, AddrMode::R_HLI, RegType::HL, RegType::A, CondType::NONE, 0, 8);
    instructions[0x23] = Instruction(Type::INC, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x24] = Instruction(Type::INC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x25] = Instruction(Type::DEC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x26] = Instruction(Type::LD, AddrMode::R_D8, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x27] = Instruction(Type::DAA, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x28] = Instruction(Type::JR, AddrMode::CC_D8, RegType::CC_Z, RegType::NONE, CondType::Z, 0, 12, 8);  // Branch taken: 12, not taken: 8
    instructions[0x29] = Instruction(Type::ADD, AddrMode::R_R, RegType::HL, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x2A] = Instruction(Type::LD, AddrMode::HLI_R, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x2B] = Instruction(Type::DEC, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x2C] = Instruction(Type::INC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x2D] = Instruction(Type::DEC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x2E] = Instruction(Type::LD, AddrMode::R_D8, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x2F] = Instruction(Type::CPL, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);

    // 0x30 - 0x3F
    instructions[0x30] = Instruction(Type::JR, AddrMode::CC_D8, RegType::CC_NC, RegType::NONE, CondType::NC, 0, 12, 8);  // Branch taken: 12, not taken: 8
    instructions[0x31] = Instruction(Type::LD, AddrMode::R_D16, RegType::SP, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x32] = Instruction(Type::LD, AddrMode::R_HLD, RegType::HL, RegType::A, CondType::NONE, 0, 8);
    instructions[0x33] = Instruction(Type::INC, AddrMode::R, RegType::SP, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x34] = Instruction(Type::INC, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x35] = Instruction(Type::DEC, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x36] = Instruction(Type::LD, AddrMode::MR_D8, RegType::HL, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0x37] = Instruction(Type::SCF, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x38] = Instruction(Type::JR, AddrMode::CC_D8, RegType::CC_C, RegType::NONE, CondType::C, 0, 12, 8);  // Branch taken: 12, not taken: 8
    instructions[0x39] = Instruction(Type::ADD, AddrMode::R_R, RegType::HL, RegType::SP, CondType::NONE, 0, 8);
    instructions[0x3A] = Instruction(Type::LD, AddrMode::HLD_R, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x3B] = Instruction(Type::DEC, AddrMode::R, RegType::SP, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x3C] = Instruction(Type::INC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x3D] = Instruction(Type::DEC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x3E] = Instruction(Type::LD, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0x3F] = Instruction(Type::CCF, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);

    // 0x40 - 0x4F
    instructions[0x40] = Instruction(Type::LD, AddrMode::R_R, RegType::B, RegType::B, CondType::NONE, 0, 4);
    instructions[0x41] = Instruction(Type::LD, AddrMode::R_R, RegType::B, RegType::C, CondType::NONE, 0, 4);
    instructions[0x42] = Instruction(Type::LD, AddrMode::R_R, RegType::B, RegType::D, CondType::NONE, 0, 4);
    instructions[0x43] = Instruction(Type::LD, AddrMode::R_R, RegType::B, RegType::E, CondType::NONE, 0, 4);
    instructions[0x44] = Instruction(Type::LD, AddrMode::R_R, RegType::B, RegType::H, CondType::NONE, 0, 4);
    instructions[0x45] = Instruction(Type::LD, AddrMode::R_R, RegType::B, RegType::L, CondType::NONE, 0, 4);
    instructions[0x46] = Instruction(Type::LD, AddrMode::R_MR, RegType::B, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x47] = Instruction(Type::LD, AddrMode::R_R, RegType::B, RegType::A, CondType::NONE, 0, 4);
    instructions[0x48] = Instruction(Type::LD, AddrMode::R_R, RegType::C, RegType::B, CondType::NONE, 0, 4);
    instructions[0x49] = Instruction(Type::LD, AddrMode::R_R, RegType::C, RegType::C, CondType::NONE, 0, 4);
    instructions[0x4A] = Instruction(Type::LD, AddrMode::R_R, RegType::C, RegType::D, CondType::NONE, 0, 4);
    instructions[0x4B] = Instruction(Type::LD, AddrMode::R_R, RegType::C, RegType::E, CondType::NONE, 0, 4);
    instructions[0x4C] = Instruction(Type::LD, AddrMode::R_R, RegType::C, RegType::H, CondType::NONE, 0, 4);
    instructions[0x4D] = Instruction(Type::LD, AddrMode::R_R, RegType::C, RegType::L, CondType::NONE, 0, 4);
    instructions[0x4E] = Instruction(Type::LD, AddrMode::R_MR, RegType::C, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x4F] = Instruction(Type::LD, AddrMode::R_R, RegType::C, RegType::A, CondType::NONE, 0, 4);

    // 0x50 - 0x5F
    instructions[0x50] = Instruction(Type::LD, AddrMode::R_R, RegType::D, RegType::B, CondType::NONE, 0, 4);
    instructions[0x51] = Instruction(Type::LD, AddrMode::R_R, RegType::D, RegType::C, CondType::NONE, 0, 4);
    instructions[0x52] = Instruction(Type::LD, AddrMode::R_R, RegType::D, RegType::D, CondType::NONE, 0, 4);
    instructions[0x53] = Instruction(Type::LD, AddrMode::R_R, RegType::D, RegType::E, CondType::NONE, 0, 4);
    instructions[0x54] = Instruction(Type::LD, AddrMode::R_R, RegType::D, RegType::H, CondType::NONE, 0, 4);
    instructions[0x55] = Instruction(Type::LD, AddrMode::R_R, RegType::D, RegType::L, CondType::NONE, 0, 4);
    instructions[0x56] = Instruction(Type::LD, AddrMode::R_MR, RegType::D, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x57] = Instruction(Type::LD, AddrMode::R_R, RegType::D, RegType::A, CondType::NONE, 0, 4);
    instructions[0x58] = Instruction(Type::LD, AddrMode::R_R, RegType::E, RegType::B, CondType::NONE, 0, 4);
    instructions[0x59] = Instruction(Type::LD, AddrMode::R_R, RegType::E, RegType::C, CondType::NONE, 0, 4);
    instructions[0x5A] = Instruction(Type::LD, AddrMode::R_R, RegType::E, RegType::D, CondType::NONE, 0, 4);
    instructions[0x5B] = Instruction(Type::LD, AddrMode::R_R, RegType::E, RegType::E, CondType::NONE, 0, 4);
    instructions[0x5C] = Instruction(Type::LD, AddrMode::R_R, RegType::E, RegType::H, CondType::NONE, 0, 4);
    instructions[0x5D] = Instruction(Type::LD, AddrMode::R_R, RegType::E, RegType::L, CondType::NONE, 0, 4);
    instructions[0x5E] = Instruction(Type::LD, AddrMode::R_MR, RegType::E, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x5F] = Instruction(Type::LD, AddrMode::R_R, RegType::E, RegType::A, CondType::NONE, 0, 4);

    // 0x60 - 0x6F
    instructions[0x60] = Instruction(Type::LD, AddrMode::R_R, RegType::H, RegType::B, CondType::NONE, 0, 4);
    instructions[0x61] = Instruction(Type::LD, AddrMode::R_R, RegType::H, RegType::C, CondType::NONE, 0, 4);
    instructions[0x62] = Instruction(Type::LD, AddrMode::R_R, RegType::H, RegType::D, CondType::NONE, 0, 4);
    instructions[0x63] = Instruction(Type::LD, AddrMode::R_R, RegType::H, RegType::E, CondType::NONE, 0, 4);
    instructions[0x64] = Instruction(Type::LD, AddrMode::R_R, RegType::H, RegType::H, CondType::NONE, 0, 4);
    instructions[0x65] = Instruction(Type::LD, AddrMode::R_R, RegType::H, RegType::L, CondType::NONE, 0, 4);
    instructions[0x66] = Instruction(Type::LD, AddrMode::R_MR, RegType::H, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x67] = Instruction(Type::LD, AddrMode::R_R, RegType::H, RegType::A, CondType::NONE, 0, 4);
    instructions[0x68] = Instruction(Type::LD, AddrMode::R_R, RegType::L, RegType::B, CondType::NONE, 0, 4);
    instructions[0x69] = Instruction(Type::LD, AddrMode::R_R, RegType::L, RegType::C, CondType::NONE, 0, 4);
    instructions[0x6A] = Instruction(Type::LD, AddrMode::R_R, RegType::L, RegType::D, CondType::NONE, 0, 4);
    instructions[0x6B] = Instruction(Type::LD, AddrMode::R_R, RegType::L, RegType::E, CondType::NONE, 0, 4);
    instructions[0x6C] = Instruction(Type::LD, AddrMode::R_R, RegType::L, RegType::H, CondType::NONE, 0, 4);
    instructions[0x6D] = Instruction(Type::LD, AddrMode::R_R, RegType::L, RegType::L, CondType::NONE, 0, 4);
    instructions[0x6E] = Instruction(Type::LD, AddrMode::R_MR, RegType::L, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x6F] = Instruction(Type::LD, AddrMode::R_R, RegType::L, RegType::A, CondType::NONE, 0, 4);

    // 0x70 - 0x7F 
    instructions[0x70] = Instruction(Type::LD, AddrMode::MR_R, RegType::HL, RegType::B, CondType::NONE, 0, 8);
    instructions[0x71] = Instruction(Type::LD, AddrMode::MR_R, RegType::HL, RegType::C, CondType::NONE, 0, 8);
    instructions[0x72] = Instruction(Type::LD, AddrMode::MR_R, RegType::HL, RegType::D, CondType::NONE, 0, 8);
    instructions[0x73] = Instruction(Type::LD, AddrMode::MR_R, RegType::HL, RegType::E, CondType::NONE, 0, 8);
    instructions[0x74] = Instruction(Type::LD, AddrMode::MR_R, RegType::HL, RegType::H, CondType::NONE, 0, 8);
    instructions[0x75] = Instruction(Type::LD, AddrMode::MR_R, RegType::HL, RegType::L, CondType::NONE, 0, 8);
    instructions[0x76] = Instruction(Type::HALT, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0x77] = Instruction(Type::LD, AddrMode::MR_R, RegType::HL, RegType::A, CondType::NONE, 0, 8);
    instructions[0x78] = Instruction(Type::LD, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0x79] = Instruction(Type::LD, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0x7A] = Instruction(Type::LD, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0x7B] = Instruction(Type::LD, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0x7C] = Instruction(Type::LD, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0x7D] = Instruction(Type::LD, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0x7E] = Instruction(Type::LD, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x7F] = Instruction(Type::LD, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);

    // 0x80 - 0x8F
    instructions[0x80] = Instruction(Type::ADD, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0x81] = Instruction(Type::ADD, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0x82] = Instruction(Type::ADD, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0x83] = Instruction(Type::ADD, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0x84] = Instruction(Type::ADD, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0x85] = Instruction(Type::ADD, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0x86] = Instruction(Type::ADD, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x87] = Instruction(Type::ADD, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);
    instructions[0x88] = Instruction(Type::ADC, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0x89] = Instruction(Type::ADC, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0x8A] = Instruction(Type::ADC, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0x8B] = Instruction(Type::ADC, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0x8C] = Instruction(Type::ADC, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0x8D] = Instruction(Type::ADC, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0x8E] = Instruction(Type::ADC, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x8F] = Instruction(Type::ADC, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);

    // 0x90 - 0x9F
    instructions[0x90] = Instruction(Type::SUB, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0x91] = Instruction(Type::SUB, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0x92] = Instruction(Type::SUB, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0x93] = Instruction(Type::SUB, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0x94] = Instruction(Type::SUB, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0x95] = Instruction(Type::SUB, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0x96] = Instruction(Type::SUB, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x97] = Instruction(Type::SUB, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);
    instructions[0x98] = Instruction(Type::SBC, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0x99] = Instruction(Type::SBC, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0x9A] = Instruction(Type::SBC, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0x9B] = Instruction(Type::SBC, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0x9C] = Instruction(Type::SBC, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0x9D] = Instruction(Type::SBC, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0x9E] = Instruction(Type::SBC, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0x9F] = Instruction(Type::SBC, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);

    // 0xA0 - 0xAF
    instructions[0xA0] = Instruction(Type::AND, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0xA1] = Instruction(Type::AND, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0xA2] = Instruction(Type::AND, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0xA3] = Instruction(Type::AND, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0xA4] = Instruction(Type::AND, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0xA5] = Instruction(Type::AND, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0xA6] = Instruction(Type::AND, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0xA7] = Instruction(Type::AND, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);
    instructions[0xA8] = Instruction(Type::XOR, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0xA9] = Instruction(Type::XOR, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0xAA] = Instruction(Type::XOR, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0xAB] = Instruction(Type::XOR, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0xAC] = Instruction(Type::XOR, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0xAD] = Instruction(Type::XOR, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0xAE] = Instruction(Type::XOR, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0xAF] = Instruction(Type::XOR, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);

    // 0xB0 - 0xBF
    instructions[0xB0] = Instruction(Type::OR, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0xB1] = Instruction(Type::OR, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0xB2] = Instruction(Type::OR, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0xB3] = Instruction(Type::OR, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0xB4] = Instruction(Type::OR, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0xB5] = Instruction(Type::OR, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0xB6] = Instruction(Type::OR, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0xB7] = Instruction(Type::OR, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);
    instructions[0xB8] = Instruction(Type::CP, AddrMode::R_R, RegType::A, RegType::B, CondType::NONE, 0, 4);
    instructions[0xB9] = Instruction(Type::CP, AddrMode::R_R, RegType::A, RegType::C, CondType::NONE, 0, 4);
    instructions[0xBA] = Instruction(Type::CP, AddrMode::R_R, RegType::A, RegType::D, CondType::NONE, 0, 4);
    instructions[0xBB] = Instruction(Type::CP, AddrMode::R_R, RegType::A, RegType::E, CondType::NONE, 0, 4);
    instructions[0xBC] = Instruction(Type::CP, AddrMode::R_R, RegType::A, RegType::H, CondType::NONE, 0, 4);
    instructions[0xBD] = Instruction(Type::CP, AddrMode::R_R, RegType::A, RegType::L, CondType::NONE, 0, 4);
    instructions[0xBE] = Instruction(Type::CP, AddrMode::R_MR, RegType::A, RegType::HL, CondType::NONE, 0, 8);
    instructions[0xBF] = Instruction(Type::CP, AddrMode::R_R, RegType::A, RegType::A, CondType::NONE, 0, 4);

    // 0xC0 - 0xCF
    instructions[0xC0] = Instruction(Type::RET, AddrMode::CC, RegType::CC_NZ, RegType::NONE, CondType::NZ, 0, 20, 8);  // Branch taken: 20, not taken: 8
    instructions[0xC1] = Instruction(Type::POP, AddrMode::R, RegType::BC, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0xC2] = Instruction(Type::JP, AddrMode::CC_D16, RegType::CC_NZ, RegType::NONE, CondType::NZ, 0, 16, 12);  // Branch taken: 16, not taken: 12
    instructions[0xC3] = Instruction(Type::JP, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NONE, 0, 16);
    instructions[0xC4] = Instruction(Type::CALL, AddrMode::CC_D16, RegType::CC_NZ, RegType::NONE, CondType::NZ, 0, 24, 12);  // Branch taken: 24, not taken: 12
    instructions[0xC5] = Instruction(Type::PUSH, AddrMode::R, RegType::BC, RegType::NONE, CondType::NONE, 0, 16);
    instructions[0xC6] = Instruction(Type::ADD, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xC7] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x00, 16);  // RST 0x00
    instructions[0xC8] = Instruction(Type::RET, AddrMode::CC, RegType::CC_Z, RegType::NONE, CondType::Z, 0, 20, 8);  // Branch taken: 20, not taken: 8
    instructions[0xC9] = Instruction(Type::RET, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 16);
    instructions[0xCA] = Instruction(Type::JP, AddrMode::CC_D16, RegType::CC_Z, RegType::NONE, CondType::Z, 0, 16, 12);  // Branch taken: 16, not taken: 12
    instructions[0xCB] = Instruction(Type::CB, AddrMode::D8, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // CB prefix - cycles handled by CB instruction
    instructions[0xCC] = Instruction(Type::CALL, AddrMode::CC_D16, RegType::CC_Z, RegType::NONE, CondType::Z, 0, 24, 12);  // Branch taken: 24, not taken: 12
    instructions[0xCD] = Instruction(Type::CALL, AddrMode::D16, RegType::NONE, RegType::NONE, CondType::NONE, 0, 24);
    instructions[0xCE] = Instruction(Type::ADC, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xCF] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x08, 16);  // RST 0x08

    // 0xD0 - 0xDF
    instructions[0xD0] = Instruction(Type::RET, AddrMode::CC, RegType::CC_NC, RegType::NONE, CondType::NC, 0, 20, 8);  // Branch taken: 20, not taken: 8
    instructions[0xD1] = Instruction(Type::POP, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0xD2] = Instruction(Type::JP, AddrMode::CC_D16, RegType::CC_NC, RegType::NONE, CondType::NC, 0, 16, 12);  // Branch taken: 16, not taken: 12
    instructions[0xD3] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xD4] = Instruction(Type::CALL, AddrMode::CC_D16, RegType::CC_NC, RegType::NONE, CondType::NC, 0, 24, 12);  // Branch taken: 24, not taken: 12
    instructions[0xD5] = Instruction(Type::PUSH, AddrMode::R, RegType::DE, RegType::NONE, CondType::NONE, 0, 16);
    instructions[0xD6] = Instruction(Type::SUB, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xD7] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x10, 16);  // RST 0x10
    instructions[0xD8] = Instruction(Type::RET, AddrMode::CC, RegType::CC_C, RegType::NONE, CondType::C, 0, 20, 8);  // Branch taken: 20, not taken: 8
    instructions[0xD9] = Instruction(Type::RETI, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 16);
    instructions[0xDA] = Instruction(Type::JP, AddrMode::CC_D16, RegType::CC_C, RegType::NONE, CondType::C, 0, 16, 12);  // Branch taken: 16, not taken: 12
    instructions[0xDB] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xDC] = Instruction(Type::CALL, AddrMode::CC_D16, RegType::CC_C, RegType::NONE, CondType::C, 0, 24, 12);  // Branch taken: 24, not taken: 12
    instructions[0xDD] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xDE] = Instruction(Type::SBC, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xDF] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x18, 16);  // RST 0x18

    // 0xE0 - 0xEF
    instructions[0xE0] = Instruction(Type::LDH, AddrMode::A8_R, RegType::NONE, RegType::A, CondType::NONE, 0, 12);  // LD ($FF00+a8),A
    instructions[0xE1] = Instruction(Type::POP, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0xE2] = Instruction(Type::LDH, AddrMode::MR_R, RegType::C, RegType::A, CondType::NONE, 0, 8);  // LD ($FF00+C),A
    instructions[0xE3] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xE4] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xE5] = Instruction(Type::PUSH, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    instructions[0xE6] = Instruction(Type::AND, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xE7] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x20, 16);  // RST 0x20
    instructions[0xE8] = Instruction(Type::ADD, AddrMode::R_D8, RegType::SP, RegType::NONE, CondType::NONE, 0, 16);  // ADD SP,r8
    instructions[0xE9] = Instruction(Type::JP, AddrMode::R, RegType::HL, RegType::NONE, CondType::NONE, 0, 4);  // JP (HL)
    instructions[0xEA] = Instruction(Type::LD, AddrMode::A16_R, RegType::NONE, RegType::A, CondType::NONE, 0, 16);  // LD (a16),A
    instructions[0xEB] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xEC] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xED] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xEE] = Instruction(Type::XOR, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xEF] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x28, 16);  // RST 0x28

    // 0xF0 - 0xFF
    instructions[0xF0] = Instruction(Type::LDH, AddrMode::R_A8, RegType::A, RegType::NONE, CondType::NONE, 0, 12);  // LD A,($FF00+a8)
    instructions[0xF1] = Instruction(Type::POP, AddrMode::R, RegType::AF, RegType::NONE, CondType::NONE, 0, 12);
    instructions[0xF2] = Instruction(Type::LDH, AddrMode::R_MR, RegType::A, RegType::C, CondType::NONE, 0, 8);  // LD A,($FF00+C)
    instructions[0xF3] = Instruction(Type::DI, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0xF4] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xF5] = Instruction(Type::PUSH, AddrMode::R, RegType::AF, RegType::NONE, CondType::NONE, 0, 16);
    instructions[0xF6] = Instruction(Type::OR, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xF7] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x30, 16);  // RST 0x30
    instructions[0xF8] = Instruction(Type::LD, AddrMode::HL_SPR, RegType::HL, RegType::SP, CondType::NONE, 0, 12);  // LD HL,SP+r8
    instructions[0xF9] = Instruction(Type::LD, AddrMode::R_R, RegType::SP, RegType::HL, CondType::NONE, 0, 8);  // LD SP,HL
    instructions[0xFA] = Instruction(Type::LD, AddrMode::R_A16, RegType::A, RegType::NONE, CondType::NONE, 0, 16);  // LD A,(a16)
    instructions[0xFB] = Instruction(Type::EI, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);
    instructions[0xFC] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xFD] = Instruction(Type::ERR, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0, 4);  // Invalid opcode
    instructions[0xFE] = Instruction(Type::CP, AddrMode::R_D8, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    instructions[0xFF] = Instruction(Type::RST, AddrMode::IMP, RegType::NONE, RegType::NONE, CondType::NONE, 0x38, 16);  // RST 0x38
}

void Instructions::initialize_cb_instructions() {
    // Initialize all CB instructions to NONE
    cb_instructions.fill(Instruction());
    
    // CB 0x00 - 0x0F
    // All CB instructions take 8 cycles for register operations and 16 cycles for memory operations
    cb_instructions[0x00] = Instruction(Type::RLC, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x01] = Instruction(Type::RLC, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x02] = Instruction(Type::RLC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x03] = Instruction(Type::RLC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x04] = Instruction(Type::RLC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x05] = Instruction(Type::RLC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x06] = Instruction(Type::RLC, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x07] = Instruction(Type::RLC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x08] = Instruction(Type::RRC, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x09] = Instruction(Type::RRC, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x0A] = Instruction(Type::RRC, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x0B] = Instruction(Type::RRC, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x0C] = Instruction(Type::RRC, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x0D] = Instruction(Type::RRC, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x0E] = Instruction(Type::RRC, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x0F] = Instruction(Type::RRC, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);

    // CB 0x10 - 0x1F
    cb_instructions[0x10] = Instruction(Type::RL, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x11] = Instruction(Type::RL, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x12] = Instruction(Type::RL, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x13] = Instruction(Type::RL, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x14] = Instruction(Type::RL, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x15] = Instruction(Type::RL, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x16] = Instruction(Type::RL, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x17] = Instruction(Type::RL, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x18] = Instruction(Type::RR, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x19] = Instruction(Type::RR, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x1A] = Instruction(Type::RR, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x1B] = Instruction(Type::RR, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x1C] = Instruction(Type::RR, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x1D] = Instruction(Type::RR, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x1E] = Instruction(Type::RR, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x1F] = Instruction(Type::RR, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);

    // CB 0x20 - 0x2F
    cb_instructions[0x20] = Instruction(Type::SLA, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x21] = Instruction(Type::SLA, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x22] = Instruction(Type::SLA, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x23] = Instruction(Type::SLA, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x24] = Instruction(Type::SLA, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x25] = Instruction(Type::SLA, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x26] = Instruction(Type::SLA, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x27] = Instruction(Type::SLA, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x28] = Instruction(Type::SRA, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x29] = Instruction(Type::SRA, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x2A] = Instruction(Type::SRA, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x2B] = Instruction(Type::SRA, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x2C] = Instruction(Type::SRA, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x2D] = Instruction(Type::SRA, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x2E] = Instruction(Type::SRA, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x2F] = Instruction(Type::SRA, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);

    // CB 0x30 - 0x3F
    cb_instructions[0x30] = Instruction(Type::SWAP, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x31] = Instruction(Type::SWAP, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x32] = Instruction(Type::SWAP, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x33] = Instruction(Type::SWAP, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x34] = Instruction(Type::SWAP, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x35] = Instruction(Type::SWAP, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x36] = Instruction(Type::SWAP, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x37] = Instruction(Type::SWAP, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x38] = Instruction(Type::SRL, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x39] = Instruction(Type::SRL, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x3A] = Instruction(Type::SRL, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x3B] = Instruction(Type::SRL, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x3C] = Instruction(Type::SRL, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x3D] = Instruction(Type::SRL, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x3E] = Instruction(Type::SRL, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x3F] = Instruction(Type::SRL, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);

    // CB 0x40 - 0x4F (BIT 0,r instructions)
    cb_instructions[0x40] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x41] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x42] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x43] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x44] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x45] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x46] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 12);
    cb_instructions[0x47] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    // BIT 1,r
    cb_instructions[0x48] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x49] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x4A] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x4B] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x4C] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x4D] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x4E] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 1, 12);
    cb_instructions[0x4F] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 1, 8);

    // CB 0x50 - 0x5F (BIT 2,r and BIT 3,r instructions)
    // BIT 2,r
    cb_instructions[0x50] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x51] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x52] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x53] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x54] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x55] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x56] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 2, 12);
    cb_instructions[0x57] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 2, 8);
    // BIT 3,r
    cb_instructions[0x58] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x59] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x5A] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x5B] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x5C] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x5D] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x5E] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 3, 12);
    cb_instructions[0x5F] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 3, 8);

    // CB 0x60 - 0x6F (BIT 4,r instructions)
    cb_instructions[0x60] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0x61] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0x62] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0x63] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0x64] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0x65] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0x66] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 4, 12);
    cb_instructions[0x67] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 4, 8);
    // BIT 5,r
    cb_instructions[0x68] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0x69] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0x6A] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0x6B] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0x6C] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0x6D] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0x6E] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 5, 12);
    cb_instructions[0x6F] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 5, 8);

    // CB 0x70 - 0x7F (BIT 6,r and BIT 7,r instructions)
    // BIT 6,r
    cb_instructions[0x70] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0x71] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0x72] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0x73] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0x74] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0x75] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0x76] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 6, 12);
    cb_instructions[0x77] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 6, 8);
    // BIT 7,r
    cb_instructions[0x78] = Instruction(Type::BIT, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0x79] = Instruction(Type::BIT, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0x7A] = Instruction(Type::BIT, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0x7B] = Instruction(Type::BIT, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0x7C] = Instruction(Type::BIT, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0x7D] = Instruction(Type::BIT, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0x7E] = Instruction(Type::BIT, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 7, 12);
    cb_instructions[0x7F] = Instruction(Type::BIT, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 7, 8);

    // CB 0x80 - 0x8F (RES 0,r and RES 1,r instructions)
    // RES 0,r
    cb_instructions[0x80] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x81] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x82] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x83] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x84] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x85] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0x86] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0x87] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    // RES 1,r
    cb_instructions[0x88] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x89] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x8A] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x8B] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x8C] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x8D] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0x8E] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 1, 16);
    cb_instructions[0x8F] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 1, 8);

    // CB 0x90 - 0x9F (RES 2,r and RES 3,r instructions)
    // RES 2,r
    cb_instructions[0x90] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x91] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x92] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x93] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x94] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x95] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0x96] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 2, 16);
    cb_instructions[0x97] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 2, 8);
    // RES 3,r
    cb_instructions[0x98] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x99] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x9A] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x9B] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x9C] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x9D] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0x9E] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 3, 16);
    cb_instructions[0x9F] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 3, 8);

    // CB 0xA0 - 0xAF (RES 4,r and RES 5,r instructions)
    // RES 4,r
    cb_instructions[0xA0] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xA1] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xA2] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xA3] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xA4] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xA5] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xA6] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 4, 16);
    cb_instructions[0xA7] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 4, 8);
    // RES 5,r
    cb_instructions[0xA8] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xA9] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xAA] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xAB] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xAC] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xAD] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xAE] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 5, 16);
    cb_instructions[0xAF] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 5, 8);

    // CB 0xB0 - 0xBF (RES 6,r and RES 7,r instructions)
    // RES 6,r
    cb_instructions[0xB0] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xB1] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xB2] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xB3] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xB4] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xB5] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xB6] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 6, 16);
    cb_instructions[0xB7] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 6, 8);
    // RES 7,r
    cb_instructions[0xB8] = Instruction(Type::RES, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xB9] = Instruction(Type::RES, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xBA] = Instruction(Type::RES, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xBB] = Instruction(Type::RES, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xBC] = Instruction(Type::RES, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xBD] = Instruction(Type::RES, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xBE] = Instruction(Type::RES, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 7, 16);
    cb_instructions[0xBF] = Instruction(Type::RES, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 7, 8);

    // CB 0xC0 - 0xCF (SET 0,r and SET 1,r instructions)
    // SET 0,r
    cb_instructions[0xC0] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0xC1] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0xC2] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0xC3] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0xC4] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0xC5] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 0, 8);
    cb_instructions[0xC6] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 0, 16);
    cb_instructions[0xC7] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 0, 8);
    // SET 1,r
    cb_instructions[0xC8] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0xC9] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0xCA] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0xCB] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0xCC] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0xCD] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 1, 8);
    cb_instructions[0xCE] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 1, 16);
    cb_instructions[0xCF] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 1, 8);

    // CB 0xD0 - 0xDF (SET 2,r and SET 3,r instructions)
    // SET 2,r
    cb_instructions[0xD0] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0xD1] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0xD2] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0xD3] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0xD4] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0xD5] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 2, 8);
    cb_instructions[0xD6] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 2, 16);
    cb_instructions[0xD7] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 2, 8);
    // SET 3,r
    cb_instructions[0xD8] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0xD9] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0xDA] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0xDB] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0xDC] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0xDD] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 3, 8);
    cb_instructions[0xDE] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 3, 16);
    cb_instructions[0xDF] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 3, 8);

    // CB 0xE0 - 0xEF (SET 4,r and SET 5,r instructions)
    // SET 4,r
    cb_instructions[0xE0] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xE1] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xE2] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xE3] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xE4] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xE5] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 4, 8);
    cb_instructions[0xE6] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 4, 16);
    cb_instructions[0xE7] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 4, 8);
    // SET 5,r
    cb_instructions[0xE8] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xE9] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xEA] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xEB] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xEC] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xED] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 5, 8);
    cb_instructions[0xEE] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 5, 16);
    cb_instructions[0xEF] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 5, 8);

    // CB 0xF0 - 0xFF (SET 6,r and SET 7,r instructions)
    // SET 6,r
    cb_instructions[0xF0] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xF1] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xF2] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xF3] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xF4] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xF5] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 6, 8);
    cb_instructions[0xF6] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 6, 16);
    cb_instructions[0xF7] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 6, 8);
    // SET 7,r
    cb_instructions[0xF8] = Instruction(Type::SET, AddrMode::R, RegType::B, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xF9] = Instruction(Type::SET, AddrMode::R, RegType::C, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xFA] = Instruction(Type::SET, AddrMode::R, RegType::D, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xFB] = Instruction(Type::SET, AddrMode::R, RegType::E, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xFC] = Instruction(Type::SET, AddrMode::R, RegType::H, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xFD] = Instruction(Type::SET, AddrMode::R, RegType::L, RegType::NONE, CondType::NONE, 7, 8);
    cb_instructions[0xFE] = Instruction(Type::SET, AddrMode::MR, RegType::HL, RegType::NONE, CondType::NONE, 7, 16);
    cb_instructions[0xFF] = Instruction(Type::SET, AddrMode::R, RegType::A, RegType::NONE, CondType::NONE, 7, 8);
}

