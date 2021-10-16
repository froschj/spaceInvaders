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

// load opcode lookup table
void Disassembler8080::buildMap(){
    opcodes.insert({0x00, &Disassembler8080::NOP});
    opcodes.insert({0x07, &Disassembler8080::RLC});
    opcodes.insert({0x0f, &Disassembler8080::RRC});
    opcodes.insert({0x21, &Disassembler8080::LXI_H});
    opcodes.insert({0x22, &Disassembler8080::SHLD});
    opcodes.insert({0x23, &Disassembler8080::INX_H});
    opcodes.insert({0x27, &Disassembler8080::DAA});
    opcodes.insert({0x2b, &Disassembler8080::DCX_H});
    opcodes.insert({0x32, &Disassembler8080::STA});
    opcodes.insert({0x35, &Disassembler8080::DCR_M});
    opcodes.insert({0x3a, &Disassembler8080::LDA});
    opcodes.insert({0x3c, &Disassembler8080::INR_A});
    opcodes.insert({0x3d, &Disassembler8080::DCR_A});
    opcodes.insert({0x3e, &Disassembler8080::MVI_A});
    opcodes.insert({0x46, &Disassembler8080::MOV_B_M});
    opcodes.insert({0x5f, &Disassembler8080::MOV_E_A});
    opcodes.insert({0x66, &Disassembler8080::MOV_H_M});
    opcodes.insert({0x67, &Disassembler8080::MOV_H_A});
    opcodes.insert({0x6f, &Disassembler8080::MOV_L_A});
    opcodes.insert({0x7e, &Disassembler8080::MOV_A_M});
    opcodes.insert({0xa7, &Disassembler8080::ANA_A});
    opcodes.insert({0xaf, &Disassembler8080::XRA_A});
    opcodes.insert({0xc1, &Disassembler8080::POP_B});
    opcodes.insert({0xc2, &Disassembler8080::JNZ});
    opcodes.insert({0xc3, &Disassembler8080::JMP});
    opcodes.insert({0xc5, &Disassembler8080::PUSH_B});
    opcodes.insert({0xc6, &Disassembler8080::ADI});
    opcodes.insert({0xc9, &Disassembler8080::RET});
    opcodes.insert({0xca, &Disassembler8080::JZ});
    opcodes.insert({0xcd, &Disassembler8080::CALL});
    opcodes.insert({0xd1, &Disassembler8080::POP_D});
    opcodes.insert({0xd2, &Disassembler8080::JNC});
    opcodes.insert({0xd5, &Disassembler8080::PUSH_D});
    opcodes.insert({0xda, &Disassembler8080::JC});
    opcodes.insert({0xdb, &Disassembler8080::IN});
    opcodes.insert({0xe1, &Disassembler8080::POP_H});
    opcodes.insert({0xe5, &Disassembler8080::PUSH_H});
    opcodes.insert({0xe6, &Disassembler8080::ANI});
    opcodes.insert({0xf1, &Disassembler8080::POP_PSW});
    opcodes.insert({0xf5, &Disassembler8080::PUSH_PSW});
    opcodes.insert({0xfb, &Disassembler8080::EI});
    opcodes.insert({0xfe, &Disassembler8080::CPI});
}

void Disassembler8080::currentAddress() {
    std::ios saveFormat(nullptr);
    saveFormat.copyfmt(outputDevice);
    outputDevice << HEX_SIGIL 
        << std::right << std::setw(4) << std::hex << std::setfill('0')
        << static_cast<int>(state.pc) << " ";
    outputDevice.copyfmt(saveFormat);
}
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

void Disassembler8080::oneByteOperand(const uint16_t address) {
    outputDevice << HEX_SIGIL 
        << std::right << std::setw(2) << std::hex << std::setfill('0') 
        << static_cast<int>(memory->read(address));
}

void Disassembler8080::mnemonic(const std::string mnemonic) {
    std::ios saveFormat(nullptr);
    saveFormat.copyfmt(outputDevice);

    outputDevice << std::left << std::setw(MNEMONIC_WIDTH) << std::setfill(' ')
        << mnemonic; 
    outputDevice.copyfmt(saveFormat);
}
/*
 * Opcode functions
 */

// disassemble the NOP opcode
int Disassembler8080::NOP() {
    mnemonic("NOP");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

// disassemble the RLC opcode
int Disassembler8080::RLC() {
    mnemonic("RLC");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

// disassemble the RRC opcode
int Disassembler8080::RRC() {
    mnemonic("RRC");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the INX H opcode
int Disassembler8080::INX_H() {
    mnemonic("INX");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the LXI H opcode
int Disassembler8080::LXI_H() {
    mnemonic("LXI");
    outputDevice << "H," << IMM_SIGIL;
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the DCX H opcode
int Disassembler8080::DCX_H() {
    mnemonic("DCX");
    outputDevice << "H" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the SHLD opcode
int Disassembler8080::SHLD() {
    mnemonic("SHLD");
    // address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice<< std::endl;

    state.pc += 3; // advance the pc correctly

    return 16;
}

// disassemble the DAA opcode
int Disassembler8080::DAA() {
    mnemonic("DAA");
    outputDevice << std::endl;

    ++state.pc;

    return 4; 
}

//disassemble the STA opcode
int Disassembler8080::STA() {
    mnemonic("STA");
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 13;
}

//disassemble the DCR_M opcode
int Disassembler8080::DCR_M() {
    mnemonic("DCR");
    outputDevice << "M" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the LDA opcode
int Disassembler8080::LDA() {
    mnemonic("LDA");
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 13;
}

//disassemble the INR_A opcode
int Disassembler8080::INR_A() {
    mnemonic("INR");
    outputDevice << "A" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the DCR_A opcode
int Disassembler8080::DCR_A() {
    mnemonic("DCR");
    outputDevice << "A" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MVI A opcode
int Disassembler8080::MVI_A() {
    mnemonic("MVI");
    outputDevice << "A," << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the MOV B,M opcode
int Disassembler8080::MOV_B_M() {
    mnemonic("MOV");
    outputDevice << "B,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV E,A opcode
int Disassembler8080::MOV_E_A() {
    mnemonic("MOV");
    outputDevice << "E,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV H,M opcode
int Disassembler8080::MOV_H_M() {
    mnemonic("MOV");
    outputDevice << "H,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the MOV H,A opcode
int Disassembler8080::MOV_H_A() {
    mnemonic("MOV");
    outputDevice << "H,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV L,A opcode
int Disassembler8080::MOV_L_A() {
    mnemonic("MOV");
    outputDevice << "L,A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 5;
}

//disassemble the MOV A,M opcode
int Disassembler8080::MOV_A_M() {
    mnemonic("MOV");
    outputDevice << "A,M" << std::endl;

    ++state.pc; // advance the pc correctly

    return 7;
}

//disassemble the ANA A opcode
int Disassembler8080::ANA_A() {
    mnemonic("ANA");    
    outputDevice << "A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the XRA A opcode
int Disassembler8080::XRA_A() {
    mnemonic("XRA");
    outputDevice << "A" << std::endl;

    ++state.pc; // advance the pc correctly

    return 4;
}

//disassemble the POP B opcode
int Disassembler8080::POP_B() {
    mnemonic("POP");
    outputDevice << "B" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the JNZ opcode
int Disassembler8080::JNZ() {
    mnemonic("JNZ");
    // address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice<< std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the JMP opcode
int Disassembler8080::JMP() {
    mnemonic("JMP");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the PUSH B opcode
int Disassembler8080::PUSH_B() {
    mnemonic("PUSH");
    outputDevice << "B" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the ADI opcode
int Disassembler8080::ADI() {
    mnemonic("ADI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

// disassemble the RET opcode
int Disassembler8080::RET() {
    mnemonic("RET");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the JZ opcode
int Disassembler8080::JZ() {
    mnemonic("JZ");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the CALL opcode
int Disassembler8080::CALL() {
    mnemonic("CALL");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 17;
}

//disassemble the POP D opcode
int Disassembler8080::POP_D() {
    mnemonic("POP");
    outputDevice << "D" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the JNC opcode
int Disassembler8080::JNC() {
    mnemonic("JNC");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the PUSH D opcode
int Disassembler8080::PUSH_D() {
    mnemonic("POP");
    outputDevice << "D" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the JC opcode
int Disassembler8080::JC() {
    mnemonic("JC");
    // get the address to jump to
    twoByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 3; // advance the pc correctly

    return 10;
}

//disassemble the IN opcode
int Disassembler8080::IN() {
    mnemonic("IN");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 10;
}

//disassemble the POP H opcode
int Disassembler8080::POP_H() {
    mnemonic("POP");
    outputDevice << "H" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the PUSH H opcode
int Disassembler8080::PUSH_H() {
    mnemonic("PUSH");
    outputDevice << "H" << std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

//disassemble the ANI opcode
int Disassembler8080::ANI() {
    mnemonic("ANI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc+1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

//disassemble the POP PSW opcode
int Disassembler8080::POP_PSW() {
    mnemonic("PUSH");
    outputDevice << "PSW" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 10;
}

//disassemble the PUSH PSW opcode
int Disassembler8080::PUSH_PSW() {
    mnemonic("PUSH");
    outputDevice << "PSW" <<std::endl;
    
    ++state.pc; // advance the pc correctly

    return 11;
}

// disassemble the EI opcode
int Disassembler8080::EI() {
    mnemonic("EI");
    outputDevice << std::endl;

    ++state.pc;

    return 4; // irrelevant for disassembler
}

//disassemble the CPI opcode
int Disassembler8080::CPI() {
    mnemonic("CPI");
    outputDevice << IMM_SIGIL;
    oneByteOperand(state.pc + 1);
    outputDevice << std::endl;

    state.pc += 2; // advance the pc correctly

    return 7;
}

