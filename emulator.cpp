

#include <functional>
#include <map>
#include <sstream>
#include <iomanip>
#include "memory.hpp"
#include "processor.hpp"
#include "emulator.hpp"



// Build an Emulator8080 with no memory attached
Emulator8080::Emulator8080() {
    this->memory = nullptr;
    this->reset(0x0000);
    this->buildMap();
}

// Build an emulator with attached memory device
Emulator8080::Emulator8080(Memory *memoryDevice) {
    this->memory = memoryDevice;
    this->reset(0x0000);
    this->buildMap();
}

// empty destructor
Emulator8080::~Emulator8080() {

}

// set the processor to a certain memory address for execution
void Emulator8080::reset(uint16_t address) {
    this->state.pc = address;
}

// fetch, decode, and execute a single instruction
int Emulator8080::step() {
    // fetch
    uint8_t opcodeWord = fetch(state.pc);
    // decode
    auto opcodeFunction = decode(opcodeWord);
    // execute
    return opcodeFunction();
}

// read an opcode in from memory
uint8_t Emulator8080::fetch(uint16_t address) {
    try { 
        return memory->read(address);
    } catch (const std::out_of_range& oor) {
        // convert to a more meaningful exception
        std::stringstream badAddress;
        badAddress << "$" 
            << std::setw(4) << std::hex << std::setfill('0')
            << static_cast<int>(address);
        std::string message(badAddress.str());
        throw MemoryReadError(message);
    }
}

// obtain host machine code corresponding to an opcode
std::function<int(void)> Emulator8080::decode(uint8_t word) {
    try {
        return opcodes.at(word);
    } catch (const std::out_of_range& oor) {
        //convert to a more meaningful exception
        std::stringstream badAddress;
        std::stringstream badOpcode;
        badAddress << "$" 
            << std::setw(4) << std::hex << std::setfill('0') 
            << static_cast<int>(state.pc);
        badOpcode << "$" 
            << std::setw(2) << std::hex << std::setfill('0') 
            << static_cast<int>(word);
        throw UnimplementedInstructionError(
            badAddress.str(),
            badOpcode.str()
        );
    }
}

void Emulator8080::buildMap() {
    // NOP (0x00): 
    // 4 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x00, 
        [this](){ 
            ++this->state.pc; 
            return 4; 
        } 
    } );
    // MVI B (0x06): B <- byte 2
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x06, 
        [this](){ 
            this->moveImmediateData(
                this->state.b, 
                memory->read(this->state.pc + 1)
            ); 
            this->state.pc +=2;
            return 7; 
        } 
    } );
    // LXI D (0x11) D <- byte 3, E <- byte 2: 
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x11, 
        [this](){
            this->state.d = this->memory->read(this->state.pc + 2);
            this->state.e = this->memory->read(this->state.pc + 1); 
            this->state.pc += 3;
            return 10; 
        } 
    } );
    // LDAX D (0x1a) A <- (DE): 
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x1a, 
        [this](){
            uint16_t msb = this->state.d << 8;
            uint16_t lsb = this->state.e;
            this->state.a = this->memory->read(msb + lsb);
            ++this->state.pc;
            return 7; 
        } 
    } );
    // LXI H (0x21) H <- byte 3, L <- byte 2: 
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x21, 
        [this](){
            this->state.h = this->memory->read(this->state.pc + 2);
            this->state.l = this->memory->read(this->state.pc + 1); 
            this->state.pc += 3;
            return 10; 
        } 
    } );
    // LXI SP (0x31) SP.hi <- byte 3, SP.lo <- byte 2: 
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x31, 
        [this](){
            uint16_t newStackPointer = this->readAddress(this->state.pc + 1); 
            this->state.sp = newStackPointer; 
            this->state.pc += 3;
            return 10; 
        } 
    } );
    // MOV M,A (0x77) (HL) <- A:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x77, 
        [this](){
            this->memory->write(this->state.a, this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // JMP (0xc3) PC <= adr: 
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xc3, 
        [this](){
            uint16_t jumpAddress = this->readAddress(this->state.pc + 1); 
            this->state.pc = jumpAddress; 
            return 10; 
        } 
    } );
    // CALL (0xcd) (SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP-2;PC=adr: 
    // 17 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xcd, 
        [this](){
            // destination address
            uint16_t callAddress = this->readAddress(this->state.pc + 1);
            // advance pc to next instruction
            this->state.pc += 3;
            // push high byte of next pc
            --this->state.sp;
            this->memory->write(
                static_cast<uint8_t>((this->state.pc & 0xFF00) >> 8),
                this->state.sp
            );
            // push low byte of next pc
            --this->state.sp;
            this->memory->write(
                static_cast<uint8_t>(this->state.pc & 0x00FF),
                this->state.sp
            );
            // go to call address next  
            this->state.pc = callAddress; 
            return 17; 
        } 
    } );
}

// read an address stored starting at atAddress
uint16_t Emulator8080::readAddress(uint16_t atAddress) {
    uint16_t lsb = this->memory->read(atAddress);
    uint16_t msb = this->memory->read(atAddress + 1) << 8;
    return msb + lsb;
}

void Emulator8080::moveImmediateData(uint8_t &destination, uint8_t data) {
    destination = data;
}

uint16_t Emulator8080::getHL() {
    uint16_t msb = this->state.h << 8;
    uint16_t lsb = this->state.l;
    return msb + lsb;
}