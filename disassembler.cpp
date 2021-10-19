/*
 * Implentation of the 8080 Disassembler "pseudoprocessor"
 */

#include "processor.hpp"
#include "disassembler.hpp"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <string>

#define MNEMONIC_WIDTH 7
#define RAW_WIDTH 12
#define HEX_SIGIL '$'
#define IMM_SIGIL '#'


// Base constructor
Disassembler8080::Disassembler8080(std::ostream &os) : 
        outputDevice(os) {
    this->memory = nullptr;
    this->reset(0x0000);
    this->buildMap();
}

// construct with memory attached
Disassembler8080::Disassembler8080(
        Memory *memoryDevice, 
        std::ostream &os) : 
        outputDevice(os) {
    memory = memoryDevice;
    this->reset(0x0000);
    this->buildMap();
}

// empty destructor
Disassembler8080::~Disassembler8080() {

}

// "execute" an instrution and return # of CPU cycles
int Disassembler8080::step() {
    // fetch
    uint8_t opcodeWord = fetch(state.pc);
    // decode
    OpcodePtr opcodeFunction = decode(opcodeWord);
    // execute
    currentAddress();
    return (this->*opcodeFunction)();
}

// set execution point
void Disassembler8080::reset(uint16_t address) {
    state.pc = address;
}

// get contents of a memory address
uint8_t Disassembler8080::fetch(uint16_t address) {
    try { 
        return memory->read(address);
    } catch (const std::out_of_range& oor) {
        // convert to a more meaningful exception
        std::stringstream badAddress;
        badAddress << HEX_SIGIL 
            << std::setw(4) << std::hex << std::setfill('0')
            << static_cast<int>(address);
        std::string message(badAddress.str());
        throw MemoryReadError(message);
    }
}

// get the host code to run for a given opcode (word)
Disassembler8080::OpcodePtr Disassembler8080::decode(uint8_t word) {
    try {
        return opcodes.at(word);
    } catch (const std::out_of_range& oor) {
        //convert to a more meaningful exception
        std::stringstream badAddress;
        std::stringstream badOpcode;
        badAddress << HEX_SIGIL 
            << std::setw(4) << std::hex << std::setfill('0') 
            << static_cast<int>(state.pc);
        badOpcode << HEX_SIGIL 
            << std::setw(2) << std::hex << std::setfill('0') 
            << static_cast<int>(word);
        throw UnimplememntedInstructionError(
            badAddress.str(),
            badOpcode.str()
        );
    }
}

//format and print the current program address
void Disassembler8080::currentAddress() {
    std::ios saveFormat(nullptr);
    saveFormat.copyfmt(outputDevice);
    outputDevice  
        << std::right << std::setw(4) << std::hex << std::setfill('0')
        << static_cast<int>(state.pc) << ": ";
    outputDevice.copyfmt(saveFormat);
}

//format and print a 2-byte operand
void Disassembler8080::twoByteOperand(const uint16_t startAddress) {
    std::ios saveFormat(nullptr);
    saveFormat.copyfmt(outputDevice);
    uint8_t lsb = memory->read(startAddress);
    uint8_t msb = memory->read(startAddress + 1);
    outputDevice << HEX_SIGIL 
        << std::right << std::setw(2) << std::hex << std::setfill('0') 
        << static_cast<int>(msb)
        << std::right << std::setw(2) << std::hex << std::setfill('0') 
        << static_cast<int>(lsb);
    outputDevice.copyfmt(saveFormat);
}

//format and print a 1-byte operand
void Disassembler8080::oneByteOperand(const uint16_t address) {
    outputDevice << HEX_SIGIL 
        << std::right << std::setw(2) << std::hex << std::setfill('0') 
        << static_cast<int>(memory->read(address));
}

// format and print a mnemonic
void Disassembler8080::mnemonic(const std::string mnemonic) {
    std::ios saveFormat(nullptr);
    saveFormat.copyfmt(outputDevice);

    outputDevice << std::left << std::setw(MNEMONIC_WIDTH) << std::setfill(' ')
        << mnemonic; 
    outputDevice.copyfmt(saveFormat);
}

// format and print the raw bytes starting at address
void Disassembler8080::instructionBytes(const uint16_t address, const int words){
    std::ios saveFormat(nullptr);
    saveFormat.copyfmt(outputDevice);

    std::stringstream rawBytes;

    for (int i = 0; i < words; ++i) {
        rawBytes << std::right << std::setw(2) << std::hex << std::setfill('0') 
            << static_cast<int>(memory->read(address + i)) << " ";
    }

    outputDevice << std::setw(RAW_WIDTH) << std::left << std::setfill(' ');
    outputDevice << rawBytes.str();

    outputDevice.copyfmt(saveFormat);
}


// load opcode lookup table
void Disassembler8080::buildMap(){
    opcodes.insert({0x00, &Disassembler8080::NOP});
    opcodes.insert({0x01, &Disassembler8080::LXI_B});
    opcodes.insert({0x02, &Disassembler8080::STAX_B});
    opcodes.insert({0x03, &Disassembler8080::INX_B});
    opcodes.insert({0x04, &Disassembler8080::INR_B});
    opcodes.insert({0x05, &Disassembler8080::DCR_B});
    opcodes.insert({0x06, &Disassembler8080::MVI_B});
    opcodes.insert({0x07, &Disassembler8080::RLC});
    opcodes.insert({0x08, &Disassembler8080::illegal});
    opcodes.insert({0x09, &Disassembler8080::DAD_B});
    opcodes.insert({0x0a, &Disassembler8080::LDAX_B});
    opcodes.insert({0x0b, &Disassembler8080::DCX_B});
    opcodes.insert({0x0c, &Disassembler8080::INR_C});
    opcodes.insert({0x0d, &Disassembler8080::DCR_C});
    opcodes.insert({0x0e, &Disassembler8080::MVI_C});
    opcodes.insert({0x0f, &Disassembler8080::RRC});

    opcodes.insert({0x10, &Disassembler8080::illegal});
    opcodes.insert({0x11, &Disassembler8080::LXI_D});
    opcodes.insert({0x12, &Disassembler8080::STAX_D});
    opcodes.insert({0x13, &Disassembler8080::INX_D});
    opcodes.insert({0x14, &Disassembler8080::INR_D});
    opcodes.insert({0x15, &Disassembler8080::DCR_D});
    opcodes.insert({0x16, &Disassembler8080::MVI_D});
    opcodes.insert({0x18, &Disassembler8080::illegal});
    opcodes.insert({0x19, &Disassembler8080::DAD_D});
    opcodes.insert({0x1a, &Disassembler8080::LDAX_D});
    opcodes.insert({0x1b, &Disassembler8080::DCX_D});    
    opcodes.insert({0x1c, &Disassembler8080::INR_E});    
    opcodes.insert({0x1d, &Disassembler8080::DCR_E});    
    opcodes.insert({0x1e, &Disassembler8080::MVI_E});    
    opcodes.insert({0x1f, &Disassembler8080::RAR});    

    opcodes.insert({0x20, &Disassembler8080::illegal});
    opcodes.insert({0x21, &Disassembler8080::LXI_H});
    opcodes.insert({0x22, &Disassembler8080::SHLD});
    opcodes.insert({0x23, &Disassembler8080::INX_H});
    opcodes.insert({0x24, &Disassembler8080::INR_H});
    opcodes.insert({0x25, &Disassembler8080::DCR_H});
    opcodes.insert({0x26, &Disassembler8080::MVI_H});
    opcodes.insert({0x27, &Disassembler8080::DAA});
    opcodes.insert({0x28, &Disassembler8080::illegal});
    opcodes.insert({0x29, &Disassembler8080::DAD_H});
    opcodes.insert({0x2a, &Disassembler8080::LHLD});
    opcodes.insert({0x2b, &Disassembler8080::DCX_H});
    opcodes.insert({0x2c, &Disassembler8080::INR_L});
    opcodes.insert({0x2e, &Disassembler8080::MVI_L});
    opcodes.insert({0x2f, &Disassembler8080::CMA});

    opcodes.insert({0x30, &Disassembler8080::illegal});
    opcodes.insert({0x31, &Disassembler8080::LXI_SP});
    opcodes.insert({0x32, &Disassembler8080::STA});
    opcodes.insert({0x34, &Disassembler8080::INR_M});
    opcodes.insert({0x35, &Disassembler8080::DCR_M});
    opcodes.insert({0x36, &Disassembler8080::MVI_M});
    opcodes.insert({0x37, &Disassembler8080::STC});
    opcodes.insert({0x38, &Disassembler8080::illegal});
    opcodes.insert({0x39, &Disassembler8080::DAD_SP});
    opcodes.insert({0x3a, &Disassembler8080::LDA});
    opcodes.insert({0x3c, &Disassembler8080::INR_A});
    opcodes.insert({0x3d, &Disassembler8080::DCR_A});
    opcodes.insert({0x3e, &Disassembler8080::MVI_A});
    opcodes.insert({0x3f, &Disassembler8080::CMC});

    opcodes.insert({0x40, &Disassembler8080::MOV_B_B});
    opcodes.insert({0x41, &Disassembler8080::MOV_B_C});
    opcodes.insert({0x42, &Disassembler8080::MOV_B_D});
    opcodes.insert({0x43, &Disassembler8080::MOV_B_E});
    opcodes.insert({0x44, &Disassembler8080::MOV_B_H});
    opcodes.insert({0x45, &Disassembler8080::MOV_B_L});
    opcodes.insert({0x46, &Disassembler8080::MOV_B_M});
    opcodes.insert({0x47, &Disassembler8080::MOV_B_A});
    opcodes.insert({0x48, &Disassembler8080::MOV_C_B});
    opcodes.insert({0x49, &Disassembler8080::MOV_C_C});
    opcodes.insert({0x4a, &Disassembler8080::MOV_C_D});
    opcodes.insert({0x4b, &Disassembler8080::MOV_C_E});
    opcodes.insert({0x4c, &Disassembler8080::MOV_C_H});
    opcodes.insert({0x4d, &Disassembler8080::MOV_C_L});
    opcodes.insert({0x4e, &Disassembler8080::MOV_C_M});
    opcodes.insert({0x4f, &Disassembler8080::MOV_C_A});

    opcodes.insert({0x50, &Disassembler8080::MOV_D_B});
    opcodes.insert({0x51, &Disassembler8080::MOV_D_C});
    opcodes.insert({0x54, &Disassembler8080::MOV_D_H});
    opcodes.insert({0x56, &Disassembler8080::MOV_D_M});
    opcodes.insert({0x57, &Disassembler8080::MOV_D_A});
    opcodes.insert({0x59, &Disassembler8080::MOV_E_C});
    opcodes.insert({0x5b, &Disassembler8080::MOV_E_E});
    opcodes.insert({0x5e, &Disassembler8080::MOV_E_M});
    opcodes.insert({0x5f, &Disassembler8080::MOV_E_A});

    opcodes.insert({0x60, &Disassembler8080::MOV_H_B});
    opcodes.insert({0x61, &Disassembler8080::MOV_H_C});
    opcodes.insert({0x62, &Disassembler8080::MOV_H_D});
    opcodes.insert({0x63, &Disassembler8080::MOV_H_E});
    opcodes.insert({0x64, &Disassembler8080::MOV_H_H});
    opcodes.insert({0x65, &Disassembler8080::MOV_H_L});
    opcodes.insert({0x66, &Disassembler8080::MOV_H_M});
    opcodes.insert({0x67, &Disassembler8080::MOV_H_A});
    opcodes.insert({0x68, &Disassembler8080::MOV_L_B});
    opcodes.insert({0x69, &Disassembler8080::MOV_L_C});
    opcodes.insert({0x6c, &Disassembler8080::MOV_L_H});
    opcodes.insert({0x6d, &Disassembler8080::MOV_L_L});
    opcodes.insert({0x6e, &Disassembler8080::MOV_L_M});
    opcodes.insert({0x6f, &Disassembler8080::MOV_L_A});

    opcodes.insert({0x70, &Disassembler8080::MOV_M_B});
    opcodes.insert({0x71, &Disassembler8080::MOV_M_C});
    opcodes.insert({0x72, &Disassembler8080::MOV_M_D});
    opcodes.insert({0x73, &Disassembler8080::MOV_M_E});
    opcodes.insert({0x74, &Disassembler8080::MOV_M_H});
    opcodes.insert({0x76, &Disassembler8080::HLT});
    opcodes.insert({0x77, &Disassembler8080::MOV_M_A});
    opcodes.insert({0x78, &Disassembler8080::MOV_A_B});
    opcodes.insert({0x79, &Disassembler8080::MOV_A_C});
    opcodes.insert({0x7a, &Disassembler8080::MOV_A_D});
    opcodes.insert({0x7b, &Disassembler8080::MOV_A_E});
    opcodes.insert({0x7c, &Disassembler8080::MOV_A_H});
    opcodes.insert({0x7d, &Disassembler8080::MOV_A_L});
    opcodes.insert({0x7e, &Disassembler8080::MOV_A_M});
    opcodes.insert({0x7f, &Disassembler8080::MOV_A_A});

    opcodes.insert({0x80, &Disassembler8080::ADD_B});
    opcodes.insert({0x81, &Disassembler8080::ADD_C});
    opcodes.insert({0x82, &Disassembler8080::ADD_D});
    opcodes.insert({0x83, &Disassembler8080::ADD_E});
    opcodes.insert({0x84, &Disassembler8080::ADD_H});
    opcodes.insert({0x85, &Disassembler8080::ADD_L});
    opcodes.insert({0x86, &Disassembler8080::ADD_M});
    opcodes.insert({0x88, &Disassembler8080::ADC_B});
    opcodes.insert({0x8a, &Disassembler8080::ADC_D});
    opcodes.insert({0x8b, &Disassembler8080::ADC_E});
    opcodes.insert({0x8e, &Disassembler8080::ADC_M});

    opcodes.insert({0x90, &Disassembler8080::SUB_B});
    opcodes.insert({0x94, &Disassembler8080::SUB_H});
    opcodes.insert({0x97, &Disassembler8080::SUB_A});
    opcodes.insert({0x98, &Disassembler8080::SBB_B});
    opcodes.insert({0x99, &Disassembler8080::SBB_C});
    opcodes.insert({0x9a, &Disassembler8080::SBB_D});
    opcodes.insert({0x9b, &Disassembler8080::SBB_E});
    opcodes.insert({0x9d, &Disassembler8080::SBB_L});
    opcodes.insert({0x9e, &Disassembler8080::SBB_M});

    opcodes.insert({0xa0, &Disassembler8080::ANA_B});
    opcodes.insert({0xa3, &Disassembler8080::ANA_E});
    opcodes.insert({0xa6, &Disassembler8080::ANA_M});
    opcodes.insert({0xa7, &Disassembler8080::ANA_A});
    opcodes.insert({0xa8, &Disassembler8080::XRA_B});
    opcodes.insert({0xaa, &Disassembler8080::XRA_D});
    opcodes.insert({0xaf, &Disassembler8080::XRA_A});

    opcodes.insert({0xb0, &Disassembler8080::ORA_B});
    opcodes.insert({0xb3, &Disassembler8080::ORA_E});
    opcodes.insert({0xb4, &Disassembler8080::ORA_H});
    opcodes.insert({0xb6, &Disassembler8080::ORA_M});
    opcodes.insert({0xb8, &Disassembler8080::CMP_B});
    opcodes.insert({0xbb, &Disassembler8080::CMP_E});
    opcodes.insert({0xbc, &Disassembler8080::CMP_H});
    opcodes.insert({0xbe, &Disassembler8080::CMP_M});

    opcodes.insert({0xc0, &Disassembler8080::RNZ});
    opcodes.insert({0xc1, &Disassembler8080::POP_B});
    opcodes.insert({0xc2, &Disassembler8080::JNZ});
    opcodes.insert({0xc3, &Disassembler8080::JMP});
    opcodes.insert({0xc4, &Disassembler8080::CNZ});
    opcodes.insert({0xc5, &Disassembler8080::PUSH_B});
    opcodes.insert({0xc6, &Disassembler8080::ADI});
    opcodes.insert({0xc8, &Disassembler8080::RZ});
    opcodes.insert({0xc9, &Disassembler8080::RET});
    opcodes.insert({0xca, &Disassembler8080::JZ});
    opcodes.insert({0xcc, &Disassembler8080::CZ});
    opcodes.insert({0xcd, &Disassembler8080::CALL});

    opcodes.insert({0xd0, &Disassembler8080::RNC});
    opcodes.insert({0xd1, &Disassembler8080::POP_D});
    opcodes.insert({0xd2, &Disassembler8080::JNC});
    opcodes.insert({0xd3, &Disassembler8080::OUT});
    opcodes.insert({0xd4, &Disassembler8080::CNC});
    opcodes.insert({0xd5, &Disassembler8080::PUSH_D});
    opcodes.insert({0xd6, &Disassembler8080::SUI});
    opcodes.insert({0xd8, &Disassembler8080::RC});
    opcodes.insert({0xda, &Disassembler8080::JC});
    opcodes.insert({0xdb, &Disassembler8080::IN});
    opcodes.insert({0xde, &Disassembler8080::SBI});

    opcodes.insert({0xe0, &Disassembler8080::RPO});
    opcodes.insert({0xe1, &Disassembler8080::POP_H});
    opcodes.insert({0xe2, &Disassembler8080::JPO});
    opcodes.insert({0xe3, &Disassembler8080::XTHL});
    opcodes.insert({0xe5, &Disassembler8080::PUSH_H});
    opcodes.insert({0xe6, &Disassembler8080::ANI});
    opcodes.insert({0xe9, &Disassembler8080::PCHL});
    opcodes.insert({0xeb, &Disassembler8080::XCHG});
    opcodes.insert({0xec, &Disassembler8080::CPE});
    opcodes.insert({0xee, &Disassembler8080::XRI});

    opcodes.insert({0xf0, &Disassembler8080::RP});
    opcodes.insert({0xf1, &Disassembler8080::POP_PSW});
    opcodes.insert({0xf5, &Disassembler8080::PUSH_PSW});
    opcodes.insert({0xf6, &Disassembler8080::ORI});
    opcodes.insert({0xf8, &Disassembler8080::RM});
    opcodes.insert({0xfa, &Disassembler8080::JM});
    opcodes.insert({0xfb, &Disassembler8080::EI});
    opcodes.insert({0xfc, &Disassembler8080::CM});
    opcodes.insert({0xfe, &Disassembler8080::CPI});
    opcodes.insert({0xff, &Disassembler8080::RST_7});
}

/*
 * Opcode functions
 */

// disassemble illegal opcodes
int Disassembler8080::illegal() {
    instructionBytes(state.pc, 1);
    mnemonic("???");
    outputDevice << std::endl;

    ++state.pc;

    return 0; // irrelevant for disassembler
}

// disassemble the NOP opcode
int Disassembler8080::NOP() {
    instructionBytes(state.pc, 1);
    mnemonic("NOP");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the LXI B opcode
int Disassembler8080::LXI_B() {
    instructionBytes(state.pc, 3);
    mnemonic("LXI");
    outputDevice << "B," << IMM_SIGIL;
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the STAX B opcode
int Disassembler8080::STAX_B() {
    instructionBytes(state.pc, 1);
    mnemonic("STAX");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INX B opcode
int Disassembler8080::INX_B() {
    instructionBytes(state.pc, 1);
    mnemonic("INX");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INR B opcode
int Disassembler8080::INR_B() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the DCR B opcode
int Disassembler8080::DCR_B() {
    instructionBytes(state.pc, 1);
    mnemonic("DCR");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MVI B opcode
int Disassembler8080::MVI_B() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "B," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the RLC opcode
int Disassembler8080::RLC() {
    instructionBytes(state.pc, 1);
    mnemonic("RLC");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

// disassemble the DAD B opcode
int Disassembler8080::DAD_B() {
    instructionBytes(state.pc, 1);
    mnemonic("DAD");
    outputDevice << "B" << std::endl;

    ++state.pc;

    return 10; // irrelevant for disassembler
}

// disassemble th LDAX B opcode
int Disassembler8080::LDAX_B() {
    instructionBytes(state.pc, 1);
    mnemonic("LDAX");
    outputDevice << "B" << std::endl;

    ++state.pc;

    return 10; // irrelevant for disassembler
}

//disassemble the DCX B opcode
int Disassembler8080::DCX_B() {
    instructionBytes(state.pc, 1);
    mnemonic("DCX");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INR C opcode
int Disassembler8080::INR_C() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the DCR C opcode
int Disassembler8080::DCR_C() {
    instructionBytes(state.pc, 1);
    mnemonic("DCR");
    outputDevice << "C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MVI C opcode
int Disassembler8080::MVI_C() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "C," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the LXI D opcode
int Disassembler8080::LXI_D() {
    instructionBytes(state.pc, 3);
    mnemonic("LXI");
    outputDevice << "D," << IMM_SIGIL;
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the STAX D opcode
int Disassembler8080::STAX_D() {
    instructionBytes(state.pc, 1);
    mnemonic("STAX");
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INX D opcode
int Disassembler8080::INX_D() {
    instructionBytes(state.pc, 1);
    mnemonic("INX");
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INR D opcode
int Disassembler8080::INR_D() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the DCR D opcode
int Disassembler8080::DCR_D() {
    instructionBytes(state.pc, 1);
    mnemonic("DCR");
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MVI D opcode
int Disassembler8080::MVI_D() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "D," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble th LDAX D opcode
int Disassembler8080::LDAX_D() {
    instructionBytes(state.pc, 1);
    mnemonic("LDAX");
    outputDevice << "D" << std::endl;

    ++state.pc;

    return 10; // irrelevant for disassembler
}

//disassemble the DCX D opcode
int Disassembler8080::DCX_D() {
    instructionBytes(state.pc, 1);
    mnemonic("DCX");
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INR E opcode
int Disassembler8080::INR_E() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the DCR E opcode
int Disassembler8080::DCR_E() {
    instructionBytes(state.pc, 1);
    mnemonic("DCR");
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MVI D opcode
int Disassembler8080::MVI_E() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "E," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the RAR opcode
int Disassembler8080::RAR() {
    instructionBytes(state.pc, 1);
    mnemonic("RAR");
    outputDevice << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

// disassemble the DAD D opcode
int Disassembler8080::DAD_D() {
    instructionBytes(state.pc, 1);
    mnemonic("DAD");
    outputDevice << "D" << std::endl;

    ++state.pc;

    return 10; // irrelevant for disassembler
}

// disassemble the RRC opcode
int Disassembler8080::RRC() {
    instructionBytes(state.pc, 1);
    mnemonic("RRC");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the INX H opcode
int Disassembler8080::INX_H() {
    instructionBytes(state.pc, 1);
    mnemonic("INX");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INR H opcode
int Disassembler8080::INR_H() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the DCR H opcode
int Disassembler8080::DCR_H() {
    instructionBytes(state.pc, 1);
    mnemonic("DCR");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MVI H opcode
int Disassembler8080::MVI_H() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "H," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the LXI H opcode
int Disassembler8080::LXI_H() {
    instructionBytes(state.pc, 3);
    mnemonic("LXI");
    outputDevice << "H," << IMM_SIGIL;
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the DCX H opcode
int Disassembler8080::DCX_H() {
    instructionBytes(state.pc, 1);
    mnemonic("DCX");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the INR L opcode
int Disassembler8080::INR_L() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

// disassemble the CMA opcode
int Disassembler8080::CMA() {
    instructionBytes(state.pc, 1);
    mnemonic("CMA");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the MVI L opcode
int Disassembler8080::MVI_L() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "L," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the SHLD opcode
int Disassembler8080::SHLD() {
    instructionBytes(state.pc, 3);
    mnemonic("SHLD");
    // address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice<< std::endl;

    state.pc += 3; // advance the pc correctly

    return 16;
}

// disassemble the DAA opcode
int Disassembler8080::DAA() {
    instructionBytes(state.pc, 1);
    mnemonic("DAA");
    outputDevice << std::endl;

    ++state.pc;

    return 4; 
}

// disassemble the DAD H opcode
int Disassembler8080::DAD_H() {
    instructionBytes(state.pc, 1);
    mnemonic("DAD");
    outputDevice << "H" << std::endl;

    ++state.pc;

    return 10; // irrelevant for disassembler
}

//disassemble the LHLD opcode
int Disassembler8080::LHLD() {
    instructionBytes(state.pc, 3);
    mnemonic("LHLD");
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the LXI SP opcode
int Disassembler8080::LXI_SP() {
    instructionBytes(state.pc, 3);
    mnemonic("LXI");
    outputDevice << "SP," << IMM_SIGIL;
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the STA opcode
int Disassembler8080::STA() {
    instructionBytes(state.pc, 3);
    mnemonic("STA");
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 13;
}

//disassemble the INR_M opcode
int Disassembler8080::INR_M() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "M" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the DCR_M opcode
int Disassembler8080::DCR_M() {
    instructionBytes(state.pc, 1);
    mnemonic("DCR");
    outputDevice << "M" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}


//disassemble the MVI M opcode
int Disassembler8080::MVI_M() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "M," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the STC opcode
int Disassembler8080::STC() {
    instructionBytes(state.pc, 1);
    mnemonic("STC");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

// disassemble the DAD SP opcode
int Disassembler8080::DAD_SP() {
    instructionBytes(state.pc, 1);
    mnemonic("DAD");
    outputDevice << "SP" << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the LDA opcode
int Disassembler8080::LDA() {
    instructionBytes(state.pc, 3);
    mnemonic("LDA");
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 13;
}

//disassemble the INR_A opcode
int Disassembler8080::INR_A() {
    instructionBytes(state.pc, 1);
    mnemonic("INR");
    outputDevice << "A" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the DCR_A opcode
int Disassembler8080::DCR_A() {
    instructionBytes(state.pc, 1);
    mnemonic("DCR");
    outputDevice << "A" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MVI A opcode
int Disassembler8080::MVI_A() {
    instructionBytes(state.pc, 2);
    mnemonic("MVI");
    outputDevice << "A," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the CMC opcode
int Disassembler8080::CMC() {
    instructionBytes(state.pc, 1);
    mnemonic("CMC");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the MOV B,B opcode
int Disassembler8080::MOV_B_B() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV B,C opcode
int Disassembler8080::MOV_B_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV B,D opcode
int Disassembler8080::MOV_B_D() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV B,E opcode
int Disassembler8080::MOV_B_E() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}


//disassemble the MOV B,H opcode
int Disassembler8080::MOV_B_H() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV B,L opcode
int Disassembler8080::MOV_B_L() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV B,M opcode
int Disassembler8080::MOV_B_M() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV B,A opcode
int Disassembler8080::MOV_B_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "B,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV C,B opcode
int Disassembler8080::MOV_C_B() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV C,C opcode
int Disassembler8080::MOV_C_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV C,D opcode
int Disassembler8080::MOV_C_D() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV C,E opcode
int Disassembler8080::MOV_C_E() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV C,H opcode
int Disassembler8080::MOV_C_H() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV C,L opcode
int Disassembler8080::MOV_C_L() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV C,M opcode
int Disassembler8080::MOV_C_M() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV C,A opcode
int Disassembler8080::MOV_C_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "C,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV D,B opcode
int Disassembler8080::MOV_D_B() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "D,B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV D,C opcode
int Disassembler8080::MOV_D_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "D,C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV D,H opcode
int Disassembler8080::MOV_D_H() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "D,H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV D,M opcode
int Disassembler8080::MOV_D_M() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "D,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV D,A opcode
int Disassembler8080::MOV_D_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "D,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV E,C opcode
int Disassembler8080::MOV_E_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "E,C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV E,E opcode
int Disassembler8080::MOV_E_E() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "E,E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV E,M opcode
int Disassembler8080::MOV_E_M() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "E,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV E,A opcode
int Disassembler8080::MOV_E_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "E,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV H,B opcode
int Disassembler8080::MOV_H_B() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV H,C opcode
int Disassembler8080::MOV_H_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,C" << std::endl;

    ++state.pc; // advance the pc correctly


    return 7;
}

//disassemble the MOV H,D opcode
int Disassembler8080::MOV_H_D() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV H,E opcode
int Disassembler8080::MOV_H_E() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV H,H opcode
int Disassembler8080::MOV_H_H() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV H,L opcode
int Disassembler8080::MOV_H_L() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV H,M opcode
int Disassembler8080::MOV_H_M() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV H,A opcode
int Disassembler8080::MOV_H_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "H,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

int Disassembler8080::MOV_L_B() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "L,B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

int Disassembler8080::MOV_L_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "L,C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

int Disassembler8080::MOV_L_H() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "L,H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

int Disassembler8080::MOV_L_L() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "L,L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

int Disassembler8080::MOV_L_M() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "L,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV L,A opcode
int Disassembler8080::MOV_L_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "L,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}



//disassemble the MOV M,B opcode
int Disassembler8080::MOV_M_B() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "M,B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV M,C opcode
int Disassembler8080::MOV_M_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "M,C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV M,D opcode
int Disassembler8080::MOV_M_D() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "M,D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV M,E opcode
int Disassembler8080::MOV_M_E() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "M,E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV M,E opcode
int Disassembler8080::MOV_M_H() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "M,H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the HLT opcode
int Disassembler8080::HLT() {
    instructionBytes(state.pc, 1);
    mnemonic("HLT");
    outputDevice << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV M,A opcode
int Disassembler8080::MOV_M_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "M,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,B opcode
int Disassembler8080::MOV_A_B() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,D opcode
int Disassembler8080::MOV_A_C() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,D opcode
int Disassembler8080::MOV_A_D() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,E opcode
int Disassembler8080::MOV_A_E() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,H opcode
int Disassembler8080::MOV_A_H() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,L opcode
int Disassembler8080::MOV_A_L() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,M opcode
int Disassembler8080::MOV_A_M() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV A,A opcode
int Disassembler8080::MOV_A_A() {
    instructionBytes(state.pc, 1);
    mnemonic("MOV");
    outputDevice << "A,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the ADD B opcode
int Disassembler8080::ADD_B() {
    instructionBytes(state.pc, 1);
    mnemonic("ADD");    
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADD C opcode
int Disassembler8080::ADD_C() {
    instructionBytes(state.pc, 1);
    mnemonic("ADD");    
    outputDevice << "C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADD D opcode
int Disassembler8080::ADD_D() {
    instructionBytes(state.pc, 1);
    mnemonic("ADD");    
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADD E opcode
int Disassembler8080::ADD_E() {
    instructionBytes(state.pc, 1);
    mnemonic("ADD");    
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADD H opcode
int Disassembler8080::ADD_H() {
    instructionBytes(state.pc, 1);
    mnemonic("ADD");    
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADD_L opcode
int Disassembler8080::ADD_L() {
    instructionBytes(state.pc, 1);
    mnemonic("ADD");    
    outputDevice << "L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADD_M opcode
int Disassembler8080::ADD_M() {
    instructionBytes(state.pc, 1);
    mnemonic("ADD");    
    outputDevice << "M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADC B opcode
int Disassembler8080::ADC_B() {
    instructionBytes(state.pc, 1);
    mnemonic("ADC");    
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADC_D opcode
int Disassembler8080::ADC_D() {
    instructionBytes(state.pc, 1);
    mnemonic("ADC");    
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADC_E opcode
int Disassembler8080::ADC_E() {
    instructionBytes(state.pc, 1);
    mnemonic("ADC");    
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ADC_M opcode
int Disassembler8080::ADC_M() {
    instructionBytes(state.pc, 1);
    mnemonic("ADC");    
    outputDevice << "M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SUB B opcode
int Disassembler8080::SUB_B() {
    instructionBytes(state.pc, 1);
    mnemonic("SUB");    
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SUB H opcode
int Disassembler8080::SUB_H() {
    instructionBytes(state.pc, 1);
    mnemonic("SUB");    
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SUB A opcode
int Disassembler8080::SUB_A() {
    instructionBytes(state.pc, 1);
    mnemonic("SUB");    
    outputDevice << "A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SBB B opcode
int Disassembler8080::SBB_B() {
    instructionBytes(state.pc, 1);
    mnemonic("SBB");    
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SBB C opcode
int Disassembler8080::SBB_C() {
    instructionBytes(state.pc, 1);
    mnemonic("SBB");    
    outputDevice << "C" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SBB D opcode
int Disassembler8080::SBB_D() {
    instructionBytes(state.pc, 1);
    mnemonic("SBB");    
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SBB E opcode
int Disassembler8080::SBB_E() {
    instructionBytes(state.pc, 1);
    mnemonic("SBB");    
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SBB E opcode
int Disassembler8080::SBB_L() {
    instructionBytes(state.pc, 1);
    mnemonic("SBB");    
    outputDevice << "L" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the SBB M opcode
int Disassembler8080::SBB_M() {
    instructionBytes(state.pc, 1);
    mnemonic("SBB");    
    outputDevice << "M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ANA B opcode
int Disassembler8080::ANA_B() {
    instructionBytes(state.pc, 1);
    mnemonic("ANA");    
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ANA E opcode
int Disassembler8080::ANA_E() {
    instructionBytes(state.pc, 1);
    mnemonic("ANA");    
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ANA M opcode
int Disassembler8080::ANA_M() {
    instructionBytes(state.pc, 1);
    mnemonic("ANA");    
    outputDevice << "M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ANA A opcode
int Disassembler8080::ANA_A() {
    instructionBytes(state.pc, 1);
    mnemonic("ANA");    
    outputDevice << "A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the XRA B opcode
int Disassembler8080::XRA_B() {
    instructionBytes(state.pc, 1);
    mnemonic("XRA");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the XRA D opcode
int Disassembler8080::XRA_D() {
    instructionBytes(state.pc, 1);
    mnemonic("XRA");
    outputDevice << "D" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the XRA A opcode
int Disassembler8080::XRA_A() {
    instructionBytes(state.pc, 1);
    mnemonic("XRA");
    outputDevice << "A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the ORA B opcode
int Disassembler8080::ORA_B() {
    instructionBytes(state.pc, 1);
    mnemonic("ORA");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the ORA E opcode
int Disassembler8080::ORA_E() {
    instructionBytes(state.pc, 1);
    mnemonic("ORA");
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the ORA H opcode
int Disassembler8080::ORA_H() {
    instructionBytes(state.pc, 1);
    mnemonic("ORA");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the ORA M opcode
int Disassembler8080::ORA_M() {
    instructionBytes(state.pc, 1);
    mnemonic("ORA");
    outputDevice << "M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the CMP B opcode
int Disassembler8080::CMP_B() {
    instructionBytes(state.pc, 1);
    mnemonic("CMP");
    outputDevice << "B" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the CMP E opcode
int Disassembler8080::CMP_E() {
    instructionBytes(state.pc, 1);
    mnemonic("CMP");
    outputDevice << "E" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the CMP B opcode
int Disassembler8080::CMP_H() {
    instructionBytes(state.pc, 1);
    mnemonic("CMP");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the CMP M opcode
int Disassembler8080::CMP_M() {
    instructionBytes(state.pc, 1);
    mnemonic("CMP");
    outputDevice << "M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

// disassemble the RNZ opcode
int Disassembler8080::RNZ() {
    instructionBytes(state.pc, 1);
    mnemonic("RNZ");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the POP B opcode
int Disassembler8080::POP_B() {
    instructionBytes(state.pc, 1);
    mnemonic("POP");
    outputDevice << "B" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the JNZ opcode
int Disassembler8080::JNZ() {
    instructionBytes(state.pc, 3);
    mnemonic("JNZ");
    // address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice<< std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the JMP opcode
int Disassembler8080::JMP() {
    instructionBytes(state.pc, 3);
    mnemonic("JMP");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the CNZ opcode
int Disassembler8080::CNZ() {
    instructionBytes(state.pc, 3);
    mnemonic("CNZ");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 17;
}

//disassemble the PUSH B opcode
int Disassembler8080::PUSH_B() {
    instructionBytes(state.pc, 1);
    mnemonic("PUSH");
    outputDevice << "B" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the ADI opcode
int Disassembler8080::ADI() {
    instructionBytes(state.pc, 2);
    mnemonic("ADI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the NOP opcode
int Disassembler8080::RZ() {
    instructionBytes(state.pc, 1);
    mnemonic("RZ");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

// disassemble the RET opcode
int Disassembler8080::RET() {
    instructionBytes(state.pc, 1);
    mnemonic("RET");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the JZ opcode
int Disassembler8080::JZ() {
    instructionBytes(state.pc, 3);
    mnemonic("JZ");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the CZ opcode
int Disassembler8080::CZ() {
    instructionBytes(state.pc, 3);
    mnemonic("CZ");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 17;
}

//disassemble the CALL opcode
int Disassembler8080::CALL() {
    instructionBytes(state.pc, 3);
    mnemonic("CALL");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 17;
}

// disassemble the RNC opcode
int Disassembler8080::RNC() {
    instructionBytes(state.pc, 1);
    mnemonic("RNC");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the POP D opcode
int Disassembler8080::POP_D() {
    instructionBytes(state.pc, 1);
    mnemonic("POP");
    outputDevice << "D" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the JNC opcode
int Disassembler8080::JNC() {
    instructionBytes(state.pc, 3);
    mnemonic("JNC");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the OUT opcode
int Disassembler8080::OUT() {
    instructionBytes(state.pc, 2);
    mnemonic("OUT");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 10;
}

//disassemble the CNC opcode
int Disassembler8080::CNC() {
    instructionBytes(state.pc, 3);
    mnemonic("CNC");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the PUSH D opcode
int Disassembler8080::PUSH_D() {
    instructionBytes(state.pc, 1);
    mnemonic("POP");
    outputDevice << "D" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the SUI opcode
int Disassembler8080::SUI() {
    instructionBytes(state.pc, 2);
    mnemonic("SUI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the RC opcode
int Disassembler8080::RC() {
    instructionBytes(state.pc, 1);
    mnemonic("RC");
    outputDevice << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the JC opcode
int Disassembler8080::JC() {
    instructionBytes(state.pc, 3);
    mnemonic("JC");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the IN opcode
int Disassembler8080::IN() {
    instructionBytes(state.pc, 2);
    mnemonic("IN");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 10;
}

//disassemble the SBI opcode
int Disassembler8080::SBI() {
    instructionBytes(state.pc, 2);
    mnemonic("SBI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the POP H opcode
int Disassembler8080::RPO() {
    instructionBytes(state.pc, 1);
    mnemonic("RPO");
    outputDevice <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the POP H opcode
int Disassembler8080::POP_H() {
    instructionBytes(state.pc, 1);
    mnemonic("POP");
    outputDevice << "H" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the JPO opcode
int Disassembler8080::JPO() {
    instructionBytes(state.pc, 3);
    mnemonic("JPO");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

// disassemble the XTHL opcode
int Disassembler8080::XTHL() {
    instructionBytes(state.pc, 1);
    mnemonic("XTHL");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the PUSH H opcode
int Disassembler8080::PUSH_H() {
    instructionBytes(state.pc, 1);
    mnemonic("PUSH");
    outputDevice << "H" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the ANI opcode
int Disassembler8080::ANI() {
    instructionBytes(state.pc, 2);
    mnemonic("ANI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the PCHL opcode
int Disassembler8080::PCHL() {
    instructionBytes(state.pc, 1);
    mnemonic("PCHL");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

// disassemble the XCHG opcode
int Disassembler8080::XCHG() {
    instructionBytes(state.pc, 1);
    mnemonic("XCHG");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the CPE opcode
int Disassembler8080::CPE() {
    instructionBytes(state.pc, 3);
    mnemonic("CPE");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the XRI opcode
int Disassembler8080::XRI() {
    instructionBytes(state.pc, 2);
    mnemonic("XRI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the NOP opcode
int Disassembler8080::RP() {
    instructionBytes(state.pc, 1);
    mnemonic("RP");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the POP PSW opcode
int Disassembler8080::POP_PSW() {
    instructionBytes(state.pc, 1);
    mnemonic("PUSH");
    outputDevice << "PSW" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the PUSH PSW opcode
int Disassembler8080::PUSH_PSW() {
    instructionBytes(state.pc, 1);
    mnemonic("PUSH");
    outputDevice << "PSW" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the ORI opcode
int Disassembler8080::ORI() {
    instructionBytes(state.pc, 2);
    mnemonic("ORI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the RM opcode
int Disassembler8080::RM() {
    instructionBytes(state.pc, 1);
    mnemonic("RM");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the JM opcode
int Disassembler8080::JM() {
    instructionBytes(state.pc, 3);
    mnemonic("JM");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

// disassemble the EI opcode
int Disassembler8080::EI() {
    instructionBytes(state.pc, 1);
    mnemonic("EI");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the CM opcode
int Disassembler8080::CM() {
    instructionBytes(state.pc, 3);
    mnemonic("CM");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the CPI opcode
int Disassembler8080::CPI() {
    instructionBytes(state.pc, 2);
    mnemonic("CPI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the RST opcode
int Disassembler8080::RST_7() {
    instructionBytes(state.pc, 1);
    mnemonic("RST7");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}