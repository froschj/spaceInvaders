

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
    // DCR B (0x05) B <- B-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x05, 
        [this](){
            this->state.b = this->decrement(this->state.b);
            ++this->state.pc;
            return 5;
        } 
    } );
    // MVI B (0x06): B <- byte 2:
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x06, 
        [this](){ 
            this->state.b = this->memory->read(this->state.pc + 1);
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
    // INX D (0x13) DE <- DE + 1:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x13, 
        [this](){
            uint16_t DE = this->getDE();
            ++DE;
            this->state.d = static_cast<uint8_t>((DE & 0xff00) >> 8);
            this->state.e = static_cast<uint8_t>(DE & 0x00ff);
            ++this->state.pc;
            return 5;
        } 
    } );
    // LDAX D (0x1a) A <- (DE): 
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x1a, 
        [this](){
            this->state.a = this->memory->read(this->getDE());
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
    // INX H (0x23) HL <- HL + 1:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x23, 
        [this](){
            uint16_t HL = this->getHL();
            ++HL;
            this->state.h = static_cast<uint8_t>((HL & 0xff00) >> 8);
            this->state.l = static_cast<uint8_t>(HL & 0x00ff);
            ++this->state.pc;
            return 5;
        } 
    } );
    // LXI SP (0x31) SP.hi <- byte 3, SP.lo <- byte 2: 
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x31, 
        [this](){
            uint16_t newStackPointer = this->readAddressFromMemory(this->state.pc + 1); 
            this->state.sp = newStackPointer; 
            this->state.pc += 3;
            return 10; 
        } 
    } );
    // MVI M (0x36) (HL) <- byte 2:
    // 10 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x36, 
        [this](){ 
            this->memory->write(
                this->memory->read(this->state.pc + 1),
                this->getHL()
            );
            this->state.pc += 2;
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
    // JNZ (0xc2) if NZ, PC <- adr
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xc2, 
        [this](){
            if (!this->state.isFlag(State8080::Z)) {
                uint16_t jumpAddress = this->readAddressFromMemory(this->state.pc + 1); 
                this->state.pc = jumpAddress;
            } else {
                this->state.pc += 3;
            }
            return 10; 
        } 
    } );
    // JMP (0xc3) PC <= adr: 
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xc3, 
        [this](){
            uint16_t jumpAddress = this->readAddressFromMemory(this->state.pc + 1); 
            this->state.pc = jumpAddress; 
            return 10; 
        } 
    } );
    // RET (0xc9) PC.lo <- (sp); PC.hi<-(sp+1); SP <- SP+2
    // 10 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xc9, 
        [this](){
            uint16_t jumpAddress = this->readAddressFromMemory(this->state.sp);
            this->state.sp += 2; 
            this->state.pc = jumpAddress; 
            return 10; 
        } 
    } );
    // CALL (0xcd) (SP-1)<-PC.hi;(SP-2)<-PC.lo;SP<-SP-2;PC=adr: 
    // 17 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xcd, 
        [this](){
            // read destination address do call actions
            this->callAddress(this->readAddressFromMemory(this->state.pc + 1));
            return 17; 
        } 
    } );
}

// read an address stored starting at atAddress
uint16_t Emulator8080::readAddressFromMemory(uint16_t atAddress) {
    uint16_t lsb = this->memory->read(atAddress);
    uint16_t msb = this->memory->read(atAddress + 1) << 8;
    return msb + lsb;
}

// set up return address on stack and set jump address
void Emulator8080::callAddress(uint16_t address) {
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
    this->state.pc = address;
}

// pair the BC registers into a 2-byte value
uint16_t Emulator8080::getBC() {
    uint16_t msb = this->state.b << 8;
    uint16_t lsb = this->state.c;
    return msb + lsb;
}

// pair the DE registers into a 2-byte value
uint16_t Emulator8080::getDE() {
    uint16_t msb = this->state.d << 8;
    uint16_t lsb = this->state.e;
    return msb + lsb;
}

// pair the HL registers into a 2-byte value
uint16_t Emulator8080::getHL() {
    uint16_t msb = this->state.h << 8;
    uint16_t lsb = this->state.l;
    return msb + lsb;
}

// determine state of Z flag
void Emulator8080::updateZeroFlag(uint8_t value) {
    if ( value == 0x00 ) { // compare to zero
        this->state.setFlag(State8080::Z);
    } else {
        this->state.unSetFlag(State8080::Z);
    }
}

// determine state of S flag
void Emulator8080::updateSignFlag(uint8_t value) {
    if ( value & 0x80 ) { // 0b1000'0000
        this->state.setFlag(State8080::S);
    } else {
        this->state.unSetFlag(State8080::S);
    }
}

// determine state of S flag
void Emulator8080::updateParityFlag(uint8_t value) {
    // https://www.freecodecamp.org/news/algorithmic-problem-solving-efficiently-computing-the-parity-of-a-stream-of-numbers-cd652af14643/
    value ^= value >> 4;
    value ^= value >> 2;
    value ^= value >> 1;
    if ( value & 0x01 ) { // 0b0000'0001
        this->state.unSetFlag(State8080::P); // odd parity
    } else {
        this->state.setFlag(State8080::P); // even parity
    }
}

// decrement a value, set Z S P AC flags
uint8_t Emulator8080::decrement(uint8_t value) {
    uint8_t nibble = 0x0f & value; 
    --value;

    this->updateZeroFlag(value);
    this->updateSignFlag(value);
    this->updateParityFlag(value);
    /* AC flag only set for addition on the 8080 per
    http://www.uyar.com/files/301/ch2.pdf
    if (0x10 & (nibble + 0x0f)) {
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }*/
    this->state.unSetFlag(State8080::AC);
    return value;
}