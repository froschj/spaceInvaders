/*
    public:
*        virtual void step() = 0;
*        virtual ~Processor();
*        Processor();
*        std::unique_ptr<stateType> getState() const {
            return state.clone();
        }
*        void connectMemory(memoryType *memoryDevice) {
            memory = memoryDevice;
        };
    protected:
*        stateType state;
*        memoryType *memory;
    public:
*        Disassembler8080();
*        Disassembler8080(Memory *memoryDevice);
*        void step() override;
*        void reset(uint16_t address = 0x0000);
    private:
*        typedef int (*OpcodePtr)(void);
*        uint8_t fetch(uint16_t address); //fetch instruction at address
*        OpcodePtr decode(uint8_t); //decode an opcode and get its execution
*        std::map<uint8_t, OpcodePtr> opcodes;
*        std::ostream &outptDevice;
        void buildMap();

*/

#include "processor.hpp"
#include "disassembler.hpp"
#include <iostream>
#include <exception>
#include <stdexcept>
#include <iomanip>
#include <sstream>
#include <string>

Disassembler8080::Disassembler8080(std::ostream &os) : 
        outputDevice(os) {
    this->memory = nullptr;
    this->reset(0x0000);
    this->buildMap();
}

Disassembler8080::Disassembler8080(
        Memory *memoryDevice, 
        std::ostream &os) : 
        outputDevice(os) {
    memory = memoryDevice;
    this->reset(0x0000);
    this->buildMap();
}

Disassembler8080::~Disassembler8080() {

}

int Disassembler8080::step() {
    uint8_t opcodeWord = fetch(state.pc);
    OpcodePtr opcodeFunction = decode(opcodeWord);
    return (this->*opcodeFunction)();
}

void Disassembler8080::reset(uint16_t address) {
    state.pc = address;
}

uint8_t Disassembler8080::fetch(uint16_t address) {
    try { 
        return memory->read(address);
    } catch (const std::out_of_range& oor) {
        std::stringstream badAddress;
        badAddress << "0x" << std::setw(4) << std::hex << std::setfill('0')
            << static_cast<int>(address);
        std::string message(badAddress.str());
        throw MemoryReadError(message);
    }
}

Disassembler8080::OpcodePtr Disassembler8080::decode(uint8_t word) {
    try {
        return opcodes.at(word);
    } catch (const std::out_of_range& oor) {
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

int Disassembler8080::NOP() {
    
    outputDevice << "0x" << std::setw(4) << std::hex << std::setfill('0')
        << static_cast<int>(state.pc);
    outputDevice << " NOP" << std::endl;
    ++state.pc;
    return 1;
}

void Disassembler8080::buildMap(){
    opcodes.insert({0x00, &Disassembler8080::NOP});
}

