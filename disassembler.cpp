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
        badAddress << "0x" << std::setw(4) << std::hex << std::setfill('0')
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
        badAddress << "0x" << std::setw(4) << std::hex << std::setfill('0') 
            << static_cast<int>(state.pc);
        badOpcode << "0x" << std::setw(2) << std::hex << std::setfill('0') 
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
}

/*
 * Opcode functions
 */

// disassemble the NOP opcode
int Disassembler8080::NOP() {
    outputDevice << "0x" << std::setw(4) << std::hex << std::setfill('0')
        << static_cast<int>(state.pc);
    outputDevice << " NOP" << std::endl;
    ++state.pc;
    return 1; // irrelevant for disassembler
}



