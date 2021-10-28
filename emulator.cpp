

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
    this->enableInterrupts = false;
    this->outputCallback = nullptr;
    this->inputCallback = nullptr;
}

// Build an emulator with attached memory device
Emulator8080::Emulator8080(Memory *memoryDevice) {
    this->connectMemory(memoryDevice);
    this->reset(0x0000);
    this->buildMap();
    this->enableInterrupts = false;
    this->outputCallback = nullptr;
    this->inputCallback = nullptr;
}

// connect a callback for the OUT instruction
// first argument is port address, second argument is value
void Emulator8080::connectOutput(
    std::function<void(uint8_t,uint8_t)> outputFunction
) {
    this->outputCallback = outputFunction;
}

// connect a callback for the IN instruction
// function returns value, argument is port address
void Emulator8080::connectInput(std::function<uint8_t(uint8_t)> inputFunction) {
    this->inputCallback = inputFunction;
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
            ++this->state.pc; // do nothin but advance pc
            return 4; 
        } 
    } );
    // LXI B (0x01) B <- byte 3, C <- byte 2:
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x01, 
        [this](){
            this->state.b = this->memory->read(this->state.pc + 2);
            this->state.c = this->memory->read(this->state.pc + 1); 
            this->state.pc += 3;
            return 10; 
        } 
    } );
    // STAX B (0x02) (BC) <- A:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x02, 
        [this](){ 
            this->memory->write(this->state.a, this->getBC());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // INX B (0x03) BC <- BC+1:
    // 5 cycles, l byte
    // no flags
    opcodes.insert( { 0x03, 
        [this](){
            uint16_t temp = this->getBC();
            ++temp;
            this->state.b = static_cast<uint8_t>((temp & 0xff00) >> 8);
            this->state.c = static_cast<uint8_t>(temp & 0xff);
            ++this->state.pc;
            return 5; 
        } 
    } );
    // INR B (0x04) B <- B+1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x04, 
        [this](){
            // increments and sets flags
            this->state.b = this->incrementValue(this->state.b);
            ++this->state.pc;
            return 5;
        } 
    } );
    // DCR B (0x05) B <- B-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x05, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->state.b = this->decrementValue(this->state.b);
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
    // RLC (0x07) A = A << 1; bit 0 = prev bit 7; CY = prev bit 7
    // 4 cycles, 1 byte
    // CY
    opcodes.insert( { 0x07, 
        [this](){ 
            //std::cout << std::hex << this->state.a << std::endl;
            //std::cout << this->state.isFlag(State8080::CY) << std::endl;
            uint16_t shiftRegister = static_cast<uint16_t>(this->state.a);
            shiftRegister = shiftRegister << 1;
            if (shiftRegister & 0x0100) {
                this->state.setFlag(State8080::CY);
                ++shiftRegister;
            } else {
                this->state.unSetFlag(State8080::CY);
            }
            shiftRegister &= 0x00ff;
            
            this->state.a = static_cast<uint8_t>(shiftRegister);
            //std::cout << std::hex <<this->state.a << std::endl;
            //std::cout << this->state.isFlag(State8080::CY) << std::endl;            
            ++this->state.pc;
            return 4; 
        } 
    } );
    // DAD B (0x09) HL = HL + BC:
    // 10 cycles, 1 byte
    // CY
    opcodes.insert( { 0x09, 
        [this](){
            this->doubleAddWithHLIntoHL(this->getBC());
            ++this->state.pc;
            return 10;
        } 
    } );
    // LDAX B (0x0a) A <- (BC): 
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x0a, 
        [this](){
            this->state.a = this->memory->read(this->getBC());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // DCX B (0x0b) BC = BC-1:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x0b, 
        [this](){
            uint16_t temp = this->getBC();
            --temp;
            this->state.b = static_cast<uint8_t>((temp & 0xff00) >> 8);
            this->state.c = static_cast<uint8_t>(temp & 0xff);
            ++this->state.pc;
            return 5; 
        } 
    } );
    // INR C (0x0c) C <- C+1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x0c, 
        [this](){
            // increments and sets flags
            this->state.c = this->incrementValue(this->state.c);
            ++this->state.pc;
            return 5;
        } 
    } );
    // DCR C (0x0d) C <-C-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x0d, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->state.c = this->decrementValue(this->state.c);
            ++this->state.pc;
            return 5;
        } 
    } );
    // MVI C (0x0e) C <- byte 2:
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x0e, 
        [this](){ 
            this->state.c = this->memory->read(this->state.pc + 1);
            this->state.pc +=2;
            return 7; 
        } 
    } );
    // RRC (0x0f) A = A >> 1; bit 7 = prev bit 0; CY = prev bit 0:
    // 4 cycles, 1 bytes
    // CY
    opcodes.insert( { 0x0f, 
        [this](){
            uint8_t carry = this->state.a & 0x01;
            this->state.a = (this->state.a & 0xfe) >> 1;
            carry = carry << 7;
            this->state.a += carry;
            if (this->state.a & 0x80) {
                this->state.setFlag(State8080::CY);
            } else {
                this->state.unSetFlag(State8080::CY);
            }
            ++this->state.pc;
            return 4;
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
    // STAX D (0x12) (DE) <- A:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x12, 
        [this](){ 
            this->memory->write(this->state.a, this->getDE());
            ++this->state.pc;
            return 7; 
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
    // INR D (0x14) D <- D+1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x14, 
        [this](){
            // increments and sets flags
            this->state.d = this->incrementValue(this->state.d);
            ++this->state.pc;
            return 5;
        } 
    } );
    // DCR D (0x15) D <- D-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x15, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->state.d = this->decrementValue(this->state.d);
            ++this->state.pc;
            return 5;
        } 
    } );
    // MVI D (0x16): D <- byte 2:
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x16, 
        [this](){ 
            this->state.d = this->memory->read(this->state.pc + 1);
            this->state.pc +=2;
            return 7; 
        } 
    } );
    // RAL (0x17) A = A << 1; bit 0 = prev CY; CY = prev bit 7
    // 4 cycles, 1 byte
    // CY
    opcodes.insert( { 0x17, 
        [this](){ 
            uint16_t shiftRegister = static_cast<uint16_t>(this->state.a);
            shiftRegister = shiftRegister << 1;
            if (this->state.isFlag(State8080::CY)) ++shiftRegister;
            if (shiftRegister & 0x0100) {
                this->state.setFlag(State8080::CY);
            } else {
                this->state.unSetFlag(State8080::CY);
            }
            shiftRegister &= 0x00ff;
            
            this->state.a = static_cast<uint8_t>(shiftRegister);          
            ++this->state.pc;
            return 4; 
        } 
    } );
    // DAD D (0x19) HL = HL + DE;
    // 10 cycles, 1 byte
    // CY
    opcodes.insert( { 0x19, 
        [this](){
            this->doubleAddWithHLIntoHL(this->getDE());
            ++this->state.pc;
            return 10;
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
    // DCX D (0x1b) DE = DE-1:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x1b, 
        [this](){
            uint16_t temp = this->getDE();
            --temp;
            this->state.d = static_cast<uint8_t>((temp & 0xff00) >> 8);
            this->state.e = static_cast<uint8_t>(temp & 0xff);
            ++this->state.pc;
            return 5; 
        } 
    } );
    // INR E (0x1c) E <- E+1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x1c, 
        [this](){
            // increments and sets flags
            this->state.e = this->incrementValue(this->state.e);
            ++this->state.pc;
            return 5;
        } 
    } );
    // DCR E (0x1d) D <- D-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x1d, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->state.e = this->decrementValue(this->state.e);
            ++this->state.pc;
            return 5;
        } 
    } );
    // MVI E (0x1e): E <- byte 2:
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x1e, 
        [this](){ 
            this->state.e = this->memory->read(this->state.pc + 1);
            this->state.pc +=2;
            return 7; 
        } 
    } );
    // RAR (0x1f) A = A >> 1; bit 7 = prev bit 7; CY = prev bit 0:
    // 4 cycles, 1 bytes
    // CY
    opcodes.insert( { 0x1f, 
        [this](){
            uint8_t carry = this->state.a & 0x01;
            this->state.a = (this->state.a & 0xfe) >> 1;
            if (this->state.isFlag(State8080::CY)) this->state.a += 0x80;
            if (carry) {
                this->state.setFlag(State8080::CY);
            } else {
                this->state.unSetFlag(State8080::CY);
            }
            ++this->state.pc;
            return 4;
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
    // SHLD (0x22) (adr) <-L; (adr+1)<-H:
    // 16 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x22, 
        [this](){ 
            uint16_t writeAddress = 
                this->readAddressFromMemory(this->state.pc + 1);
            this->memory->write(this->state.l, writeAddress);
            this->memory->write(this->state.h, ++writeAddress);
            this->state.pc += 3;
            return 16; 
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
    // INR H (0x04) H <- H+1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x24, 
        [this](){
            // increments and sets flags
            this->state.h = this->incrementValue(this->state.h);
            ++this->state.pc;
            return 5;
        } 
    } );
    // DCR H (0x25) H <- H-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x25, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->state.h = this->decrementValue(this->state.h);
            ++this->state.pc;
            return 5;
        } 
    } );
    // MVI H (0x26) H <- byte 2:
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x26, 
        [this](){ 
            this->state.h = this->memory->read(this->state.pc + 1);
            this->state.pc += 2;
            return 7; 
        } 
    } );
    // DAA (0x27) special:
    // 4 cycles, 1 byte
    // Z, S, P, AC, CY
    opcodes.insert( { 0x27, 
        [this](){ 
            uint8_t lowNibble = this->state.a & 0x0f;
            // add 6 to the low nibble if it is more than 9
            // or if the AC flag is set
            // set the AC flag based on this action
            if (
                (lowNibble > 0x09)
                || this->state.isFlag(State8080::AC)
            ) {
                lowNibble += 0x06;
                if (lowNibble & 0x10) {
                    this->state.setFlag(State8080::AC);
                } else {
                    this->state.unSetFlag(State8080::AC);
                }
            }
            lowNibble &= 0x0f;
            uint8_t highNibble = (this->state.a & 0xf0) >> 4;
            if (this->state.isFlag(State8080::AC)) ++highNibble;
            // add 6 to the high nibble if it is more than 9
            // or if the CY flag is set
            // set the CY flag based on the outcome
            if (
                (highNibble > 0x09)
                || this->state.isFlag(State8080::CY)
            ) {
                highNibble += 0x06;
                if (highNibble & 0x10) {
                    this->state.setFlag(State8080::CY);
                } else {
                    this->state.unSetFlag(State8080::CY);
                }
            }
            highNibble = (highNibble & 0x0f) << 4;
            this->state.a = highNibble + lowNibble;
            ++this->state.pc;
            return 4; 
        } 
    } );
    // DAD H (0x29) HL = HL + HL
    // 10 cycles, 1 byte
    // CY
    opcodes.insert( { 0x29, 
        [this](){ 
            this->doubleAddWithHLIntoHL(this->getHL());
            ++this->state.pc;
            return 10; 
        } 
    } );
    // LHLD (0x2a) L <- (adr); H<-(adr+1)
    // 16 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x2a, 
        [this](){ 
            uint16_t readAddress = 
                this->readAddressFromMemory(this->state.pc + 1);
            this->state.l = this->memory->read(readAddress);
            this->state.h = this->memory->read(++readAddress);
            this->state.pc += 3;
            return 16; 
        } 
    } );
    // DCX H (0x2b) HL = HL-1:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x2b, 
        [this](){
            uint16_t temp = this->getHL();
            --temp;
            this->state.h = static_cast<uint8_t>((temp & 0xff00) >> 8);
            this->state.l = static_cast<uint8_t>(temp & 0xff);
            ++this->state.pc;
            return 5; 
        } 
    } );
    // INR L (0x2c) L <- L+1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x2c, 
        [this](){
            // increments and sets flags
            this->state.l = this->incrementValue(this->state.l);
            ++this->state.pc;
            return 5;
        } 
    } );
    // DCR L (0x2d) L <- L-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x2d, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->state.l = this->decrementValue(this->state.l);
            ++this->state.pc;
            return 5;
        } 
    } );
    // MVI L (0x2e): L <- byte 2:
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x2e, 
        [this](){ 
            this->state.l = this->memory->read(this->state.pc + 1);
            this->state.pc +=2;
            return 7; 
        } 
    } );
    // CMA (0x2f) A <- !A:
    // 4 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x2f, 
        [this](){ 
            this->state.a = ~(this->state.a);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // LXI SP (0x31) SP.hi <- byte 3, SP.lo <- byte 2: 
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x31, 
        [this](){
            // readAddressFromMemory(uint16_t) accouts for little-endian storage
            uint16_t newStackPointer = 
                this->readAddressFromMemory(this->state.pc + 1); 
            this->state.sp = newStackPointer; 
            this->state.pc += 3;
            return 10; 
        } 
    } );
    // STA A (0x32) (adr) <- A:
    // 13 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x32, 
        [this](){
            this->memory->write(
                this->state.a,
                this->readAddressFromMemory(this->state.pc + 1)
            );
            this->state.pc += 3;
            return 13; 
        } 
    } );
    // INR M (0x34) (HL) <- (HL)+1:
    // 10 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x34, 
        [this](){
            // increments and sets flags
            this->memory->write(
                this->incrementValue(this->memory->read(this->getHL())),
                this->getHL()
            );
            ++this->state.pc;
            return 10;
        } 
    } );
    // DCR M (0x35) (HL) <- (HL)-1:
    // 10 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x35, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->memory->write(
                this->decrementValue(this->memory->read(this->getHL())),
                this->getHL()
            );
            ++this->state.pc;
            return 10;
        } 
    } );
    // MVI M (0x36) (HL) <- byte 2:
    // 10 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x36, 
        [this](){ 
            this->memory->write(
                this->memory->read(this->state.pc + 1), // immediate value
                this->getHL() // memory address in paired register
            );
            this->state.pc += 2;
            return 10; 
        } 
    } );
    // STC (0x37) CY = 1:
    // 4 cycles, 1 byte
    // CY
    opcodes.insert( { 0x37, 
        [this](){ 
            this->state.setFlag(State8080::CY);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // LDA (0x3a) A <- (adr):
    // 13 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0x3a, 
        [this](){
            this->state.a = this->memory->read(
                this->readAddressFromMemory(this->state.pc + 1)
            );
            this->state.pc += 3;
            return 13; 
        } 
    } );
    // INR A (0x3c) A <- A+1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x3c, 
        [this](){
            // increments and sets flags
            this->state.a = this->incrementValue(this->state.a);
            ++this->state.pc;
            return 5;
        } 
    } );
    // DCR A (0x3d) A <- A-1:
    // 5 cycles, 1 byte
    // Z, S, P, AC
    opcodes.insert( { 0x3d, 
        [this](){
            // decrement(uint8_t) decrements and sets flags
            this->state.a = this->decrementValue(this->state.a);
            ++this->state.pc;
            return 5;
        } 
    } );
    // MVI A (0x3e) A <- byte 2
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x3e, 
        [this](){ 
            this->state.a = this->memory->read(this->state.pc + 1);
            this->state.pc +=2;
            return 7; 
        } 
    } );
    // CMC (0x3f) CY = !CY:
    // 4 cycles, 1 byte
    // CY
    opcodes.insert( { 0x3f, 
        [this](){ 
            this->state.complementFlag(State8080::CY);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // MOV B,C (0x41) B <- C:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x41, 
        [this](){
            this->state.b = this->state.c;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV B,D (0x42) B <- D:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x42, 
        [this](){
            this->state.b = this->state.d;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV B,E (0x43) B <- E:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x43, 
        [this](){
            this->state.b = this->state.e;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV B,H (0x44) B <- H:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x44, 
        [this](){
            this->state.b = this->state.h;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV B,L (0x45) B <- L:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x45, 
        [this](){
            this->state.b = this->state.l;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV B,M (0x46) B <- (HL):
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x46, 
        [this](){
            this->state.b = this->memory->read(this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV B,A (0x47) B <- A:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x47, 
        [this](){
            this->state.b = this->state.a;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV C,B (0x48) C <- B:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x48, 
        [this](){
            this->state.c = this->state.b;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV C,D (0x4a) C <- D:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x4a, 
        [this](){
            this->state.c = this->state.d;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV C,E (0x4b) C <- E:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x4b, 
        [this](){
            this->state.c = this->state.e;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV C,H (0x4c) C <- H:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x4c, 
        [this](){
            this->state.c = this->state.h;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV C,L (0x4d) C <- L:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x4d, 
        [this](){
            this->state.c = this->state.l;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV C,A (0x4f) C <- A:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x4f, 
        [this](){
            this->state.c = this->state.a;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV D,C (0x50) D <- C:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x50, 
        [this](){
            this->state.d = this->state.c;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV D,C (0x51) D <- C:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x51, 
        [this](){
            this->state.d = this->state.c;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV D,E (0x53) D <- E:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x53, 
        [this](){
            this->state.d = this->state.e;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV D,H (0x54) D <- H:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x54, 
        [this](){
            this->state.d = this->state.h;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV D,L (0x55) D <- L:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x55, 
        [this](){
            this->state.d = this->state.l;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV D,M (0x56) D <- (HL):
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x56, 
        [this](){
            this->state.d = this->memory->read(this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV D,A (0x57) D <- A:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x57, 
        [this](){
            this->state.d = this->state.a;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV E,B (0x58) E <- B:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x58, 
        [this](){
            this->state.e = this->state.b;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV E,C (0x59) E <- C:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x59, 
        [this](){
            this->state.e = this->state.c;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV E,D (0x5a) E <- D:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x5a, 
        [this](){
            this->state.e = this->state.d;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV E,H (0x5c) E <- H:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x5c, 
        [this](){
            this->state.e = this->state.h;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV E,L (0x5d) E <- L:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x5d, 
        [this](){
            this->state.e = this->state.l;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV E,M (0x5e) E <- (HL):
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x5e, 
        [this](){
            this->state.e = this->memory->read(this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV E,A (0x5f) E <- A
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x5f, 
        [this](){
            this->state.e = this->state.a;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV H,B (0x60) H <- B:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x60, 
        [this](){
            this->state.h = this->state.b;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV H,C (0x61) H <- C:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x61, 
        [this](){
            this->state.h = this->state.c;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV H,D (0x62) H <- D:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x62, 
        [this](){
            this->state.h = this->state.d;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV H,E (0x63) H <- E:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x63, 
        [this](){
            this->state.h = this->state.e;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV H,L (0x65) H <- L:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x65, 
        [this](){
            this->state.h = this->state.l;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV H,M (0x66) H <- (HL):
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x66, 
        [this](){
            this->state.h = this->memory->read(this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV H,A (0x67) H <- A:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x67, 
        [this](){
            this->state.h = this->state.a;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV L,B (0x68) L <- B:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x68, 
        [this](){
            this->state.l = this->state.b;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV L,C (0x69) L <- C:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x69, 
        [this](){
            this->state.l = this->state.c;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV L,D (0x6a) L <- D:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x6a, 
        [this](){
            this->state.l = this->state.d;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV L,E (0x6b) L <- E:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x6b, 
        [this](){
            this->state.l = this->state.e;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV L,H (0x6c) L <- H:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x6c, 
        [this](){
            this->state.l = this->state.h;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV L,M (0x6e) L <- (HL):
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x6e, 
        [this](){
            this->state.l = this->memory->read(this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV L,A (0x6f) L <- A:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x6f, 
        [this](){
            this->state.l = this->state.a;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV M,B (0x70) (HL) <- B:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x70, 
        [this](){
            this->memory->write(this->state.b, this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV M,C (0x71) (HL) <- C:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x71, 
        [this](){
            this->memory->write(this->state.c, this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV M,D (0x72) (HL) <- D:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x72, 
        [this](){
            this->memory->write(this->state.d, this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV M,E (0x73) (HL) <- E:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x73, 
        [this](){
            this->memory->write(this->state.e, this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // MOV M,H (0x74) (HL) <- H:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x74, 
        [this](){
            this->memory->write(this->state.h, this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } ); 
    // MOV M,L (0x75) (HL) <- L:
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x75, 
        [this](){
            this->memory->write(this->state.l, this->getHL());
            ++this->state.pc;
            return 7; 
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
    // MOV A,B (0x78) A <- B:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x78, 
        [this](){
            this->state.a = this->state.b;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV A,C (0x79) A <- C:
    // 5 cycles, 1 byte
    //no flags
    opcodes.insert( { 0x79, 
        [this](){
            this->state.a = this->state.c;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV A,D (0x7a) A <- D:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x7a, 
        [this](){
            this->state.a = this->state.d;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV A,E (0x7b) A <- E:
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x7b, 
        [this](){
            this->state.a = this->state.e;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV A,H (0x7c) A <- H
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x7c, 
        [this](){
            this->state.a = this->state.h;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV A,L (0x7d) A <- L
    // 5 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x7d, 
        [this](){
            this->state.a = this->state.l;
            ++this->state.pc;
            return 5; 
        } 
    } );
    // MOV A,M (0x7e) A <- (HL):
    // 7 cycles, 1 byte
    // no flags
    opcodes.insert( { 0x7e, 
        [this](){
            this->state.a = this->memory->read(this->getHL());
            ++this->state.pc;
            return 7; 
        } 
    } );
    // ADD B (0x80) A <- A + B
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x80, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.b);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADD C (0x81) A <- A + C
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x81, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.c);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADD D (0x82) A <- A + D
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x82, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.d);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADD E (0x83) A <- A + E
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x83, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.e);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADD H (0x84) A <- A + H
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x84, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.h);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADD L (0x85) A <- A + L
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x85, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.l);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADD M (0x86) A <- A + (HL)
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x86, 
        [this](){
            this->state.a = this->addWithAccumulator(
                this->memory->read(this->getHL())
            );
            ++this->state.pc;
            return 7; 
        } 
    } );
    // ADD A (0x87) A <- A + A
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x87, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.a);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADC B (0x88) A <- A + B + CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x88, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.b, true);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADC C (0x89) A <- A + C + CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x89, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.c, true);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADC D (0x8a) A <- A + D + CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x8a, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.d, true);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADC E (0x8b) A <- A + E + CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x8b, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.e, true);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADC H (0x8c) A <- A + H + CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x8c,
        [this](){
            this->state.a = this->addWithAccumulator(this->state.h, true);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADC L (0x8d) A <- A + L + CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x8d, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.l, true);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ADC M (0x8e) A <- A + (HL) + CY
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x8e, 
        [this](){
            this->state.a = this->addWithAccumulator(
                this->memory->read(this->getHL()), true
            );
            ++this->state.pc;
            return 7; 
        } 
    } );
    // ADC A (0x8f) A <- A + B + CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x8f, 
        [this](){
            this->state.a = this->addWithAccumulator(this->state.a, true);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SUB B (0x90) A <- A - B
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x90, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->state.b);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SUB C (0x91) A <- A - C
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x91, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->state.c);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SUB D (0x92) A <- A - D
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x92, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->state.d);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SUB E (0x93) A <- A - E
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x93, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->state.e);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SUB H (0x94) A <- A - H
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x94, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->state.h);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SUB L (0x95) A <- A - L
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x95, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->state.l);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SUB M (0x96) A <- A - (HL)
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x96, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->memory->read(this->getHL()));
            ++this->state.pc;
            return 7; 
        } 
    } );
    // SUB A (0x97) A <- A - A
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x97, 
        [this](){
            this->state.a = this->subtractValues(this->state.a, this->state.a);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SBB B (0x98) A <- A - B - CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x98, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->state.b, true
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SBB C (0x99) A <- A - C - CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x99, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->state.c, true
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SBB D (0x9a) A <- A - D - CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x9a, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->state.d, true
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SBB E (0x9b) A <- A - E - CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x9b, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->state.e, true
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SBB H (0x9c) A <- A - H - CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x9c, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->state.h, true
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SBB L (0x9d) A <- A - L - CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x9d, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->state.l, true
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // SBB M (0x9e) A <- A - (HL) - CY
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x9e, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->memory->read(this->getHL()), true
            );
            ++this->state.pc;
            return 7; 
        } 
    } );
    // SBB A (0x9f) A <- A - A - CY
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0x9f, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, this->state.a, true
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ANA B (0xa0) A <- A & B
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa0, 
        [this](){
            this->state.a = this->andWithAccumulator(this->state.b);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ANA C (0xa1) A <- A & C
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa1, 
        [this](){
            this->state.a = this->andWithAccumulator(this->state.c);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ANA D (0xa2) A <- A & D
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa2, 
        [this](){
            this->state.a = this->andWithAccumulator(this->state.d);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ANA E (0xa3) A <- A & E
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa3, 
        [this](){
            this->state.a = this->andWithAccumulator(this->state.e);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ANA H (0xa4) A <- A & H
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa4, 
        [this](){
            this->state.a = this->andWithAccumulator(this->state.h);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ANA L (0xa5) A <- A & L
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa5, 
        [this](){
            this->state.a = this->andWithAccumulator(this->state.l);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ANA M (0xa6) A <- A & HL
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa6, 
        [this](){
            this->state.a = this->andWithAccumulator(
                this->memory->read(this->getHL())
            );
            ++this->state.pc;
            return 7; 
        } 
    } );
    // ANA A (0xa7) A <- A & A
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa7, 
        [this](){
            this->state.a = this->andWithAccumulator(this->state.a);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // XRA B (0xa8) A <- A ^ B
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa8, 
        [this](){
            this->state.a = this->xorWithAccumulator(this->state.b);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // XRA C (0xa9) A <- A ^ C
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xa9, 
        [this](){
            this->state.a = this->xorWithAccumulator(this->state.c);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // XRA D (0xaa) A <- A ^ D
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xaa, 
        [this](){
            this->state.a = this->xorWithAccumulator(this->state.d);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // XRA E (0xab) A <- A ^ E
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xab, 
        [this](){
            this->state.a = this->xorWithAccumulator(this->state.e);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // XRA H (0xac) A <- A ^ H
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xac, 
        [this](){
            this->state.a = this->xorWithAccumulator(this->state.h);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // XRA L (0xad) A <- A ^ L
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xad, 
        [this](){
            this->state.a = this->xorWithAccumulator(this->state.l);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // XRA M (0xae) A <- A ^ (HL)
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xae, 
        [this](){
            this->state.a = this->xorWithAccumulator(
                this->memory->read(this->getHL())
            );
            ++this->state.pc;
            return 7; 
        } 
    } );
    // XRA A (0xaf) A <- A ^ A
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xaf, 
        [this](){
            this->state.a = this->xorWithAccumulator(this->state.a);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ORA B (0xb0) A <- A | B
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb0, 
        [this](){
            this->state.a = this->orWithAccumulator(this->state.b);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ORA C (0xb1) A <- A | C
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb1, 
        [this](){
            this->state.a = this->orWithAccumulator(this->state.c);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ORA D (0xb2) A <- A | D
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb2, 
        [this](){
            this->state.a = this->orWithAccumulator(this->state.d);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ORA E (0xb3) A <- A | E
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb3, 
        [this](){
            this->state.a = this->orWithAccumulator(this->state.e);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ORA H (0xb4) A <- A | H
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb4, 
        [this](){
            this->state.a = this->orWithAccumulator(this->state.h);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ORA L (0xb5) A <- A | L
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb5, 
        [this](){
            this->state.a = this->orWithAccumulator(this->state.l);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // ORA M (0xb6) A <- A | (HL)
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb6, 
        [this](){
            this->state.a = this->orWithAccumulator(
                this->memory->read(this->getHL())
            );
            ++this->state.pc;
            return 7; 
        } 
    } );
    // ORA A (0xb7) A <- A | A
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb7, 
        [this](){
            this->state.a = this->orWithAccumulator(this->state.a);
            ++this->state.pc;
            return 4; 
        } 
    } );
    // CMP B (0xb8) A - B
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb8, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->state.b //subtrahend
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // CMP C (0xb9) A - C
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xb9, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->state.c //subtrahend
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // CMP D (0xba) A - D
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xba, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->state.d //subtrahend
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // CMP E (0xbb) A - E
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xbb, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->state.e //subtrahend
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // CMP H (0xbc) A - H
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xbc, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->state.h //subtrahend
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // CMP L (0xbd) A - C
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xbd, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->state.l //subtrahend
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // CMP M (0xbe) A - (HL):
    // 7 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xbe, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->memory->read(this->getHL()) //subtrahend
            );
            ++this->state.pc;
            return 7; 
        } 
    } );
    // CMP A (0xbf) A - A
    // 4 cycles, 1 byte
    // Z, S, P, CY, AC
    opcodes.insert( { 0xbf, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->state.a //subtrahend
            );
            ++this->state.pc;
            return 4; 
        } 
    } );
    // RNZ (0xc0) if NZ, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xc0, 
        [this](){
            if (!(this->state.isFlag(State8080::Z))) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
        } 
    } );
    // POP B (0xc1) C <- (sp); B <- (sp+1); sp <- sp+2:
    // 10 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xc1, 
        [this](){
            // pop c
            this->state.c = this->memory->read(this->state.sp++);
            // pop b
            this->state.b = this->memory->read(this->state.sp++);
            ++this->state.pc; 
            return 10; 
        } 
    } );
    // JNZ (0xc2) if NZ, PC <- adr
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xc2, 
        [this](){
            if (!this->state.isFlag(State8080::Z)) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1); 
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
            uint16_t jumpAddress = 
                this->readAddressFromMemory(this->state.pc + 1); 
            this->state.pc = jumpAddress; 
            return 10; 
        } 
    } );
    // CNZ (0xc4) if if NZ, CALL adr
    // 17 cycles if call; otherwise 11, 3 bytes
    // no flags
    opcodes.insert( { 0xc4, 
        [this](){
            // read destination address do call actions
            if (!(this->state.isFlag(State8080::Z))) {
                return (this->decode(0xcd))(); // CALL (0xcd) 17 cycles
            } else {
                this->state.pc += 3;
                return 11;
            }
            
        } 
    } );
    // PUSH B (0xc5) (sp-2)<-C; (sp-1)<-B; sp <- sp - 2:
    // 11 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xc5, 
        [this](){
            // push b
            this->memory->write(this->state.b, --this->state.sp);
            // push c
            this->memory->write(this->state.c, --this->state.sp);
            ++this->state.pc;
            return 11; 
        } 
    } );
    // ADI (0xc6) A <- A + byte:
    // 7 cycles, 2 bytes
    // Z, S, P, CY, AC
    opcodes.insert( { 0xc6, 
        [this](){
            this->state.a = 
                this->addWithAccumulator(
                    this->memory->read(this->state.pc + 1)
                );
            this->state.pc += 2;
            return 7; 
        } 
    } );
    // RZ (0xc8) if Z, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xc8, 
        [this](){
            if (this->state.isFlag(State8080::Z)) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
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
    // JZ (0xca) if Z, PC <- adr
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xca, 
        [this](){
            if (this->state.isFlag(State8080::Z)) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1);
                this->state.pc = jumpAddress; 
            } else {
                this->state.pc += 3;
            }
            return 10;
        } 
    } );
    // CZ (0xcc) if if Z, CALL adr
    // 17 cycles if call; otherwise 11, 3 bytes
    // no flags
    opcodes.insert( { 0xcc, 
        [this](){
            // read destination address do call actions
            if (this->state.isFlag(State8080::Z)) {
                return (this->decode(0xcd))(); // CALL (0xcd) 17 cycles
            } else {
                this->state.pc += 3;
                return 11;
            }
            
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
    // ACI (0xce) A <- A + data + CY:
    // 7 cycles, 2 bytes
    // Z, S, P, CY, AC
    opcodes.insert( { 0xce, 
        [this](){
            this->state.a = 
                this->addWithAccumulator(
                    this->memory->read(this->state.pc + 1), true
                );
            this->state.pc += 2;
            return 7; 
        } 
    } );
    // RNC (0xd0) if NCY, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xd0, 
        [this](){
            if (!(this->state.isFlag(State8080::CY))) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
        } 
    } );
    // POP D (0xd1) E <- (sp); D <- (sp+1); sp <- sp+2:
    // 10 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xd1, 
        [this](){
            // pop e
            this->state.e = this->memory->read(this->state.sp++);
            // pop d
            this->state.d = this->memory->read(this->state.sp++);
            ++this->state.pc; 
            return 10; 
        } 
    } );
    // JNC (0xd2) if NCY, PC<-adr
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xd2, 
        [this](){
            if (!(this->state.isFlag(State8080::CY))) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1);
                this->state.pc = jumpAddress; 
            } else {
                this->state.pc += 3;
            }
            return 10;
        } 
    } );
    // OUT (0xd3) special
    // 10 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0xd3, 
        [this](){
            // call callback with port, value
            outputCallback(
                this->memory->read(this->state.pc + 1), 
                this->state.a
            );
            this->state.pc += 2;
            return 10; 
        } 
    } );
    // CNC (0xd4) if NCY, CALL adr
    // 17 cycles if call; otherwise 11, 3 bytes
    // no flags
    opcodes.insert( { 0xd4, 
        [this](){
            // read destination address do call actions
            if (!(this->state.isFlag(State8080::CY))) {
                return (this->decode(0xcd))(); // CALL (0xcd) 17 cycles
            } else {
                this->state.pc += 3;
                return 11;
            }
        } 
    } );
    // PUSH D (0xd5) (sp-2)<-E; (sp-1)<-D; sp <- sp - 2:
    // 11 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xd5, 
        [this](){
            // push d
            this->memory->write(this->state.d, --this->state.sp);
            // push e
            this->memory->write(this->state.e, --this->state.sp);
            ++this->state.pc;
            return 11; 
        } 
    } );
    // SUI (0xd6) A <- A - data
    // 7 cycles, 2 bytes
    // Z, S, P, CY, AC
    opcodes.insert( { 0xd6, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, // minuend
                this->memory->read(this->state.pc + 1) //subtrahend
            );
            this->state.pc += 2;
            return 7; 
        } 
    } );
    // RC (0xd8) if CY, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xd8, 
        [this](){
            if (this->state.isFlag(State8080::CY)) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
        } 
    } );
    // JC (0xda) if CY, PC<-adr
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xda, 
        [this](){
            if (this->state.isFlag(State8080::CY)) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1);
                this->state.pc = jumpAddress; 
            } else {
                this->state.pc += 3;
            }
            return 10;
        } 
    } );
    // CC if CY, CALL adr
    // 17 cycles on CALL; 11 otherwise, 3 bytes
    // no flags
    opcodes.insert( { 0xdc, 
        [this](){
            // read destination address do call actions
            if (this->state.isFlag(State8080::CY)) {
                return(this->decode(0xcd))(); // CALL (0xcd) 17 cycles 
            } else {
                this->state.pc += 3;
                return 11;
            }
            
        } 
    } );
    // SBI (0xde) A <- A - data - CY
    // 7 cycles, 2 bytes
    // Z, S, P, CY, AC
    opcodes.insert( { 0xde, 
        [this](){
            this->state.a = this->subtractValues(
                this->state.a, // minuend
                this->memory->read(this->state.pc + 1), //subtrahend
                true //include carry bit
            );
            this->state.pc += 2;
            return 7; 
        } 
    } );
    // RPO (0xe0) if PO, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xe0, 
        [this](){
            if (!(this->state.isFlag(State8080::P))) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
        } 
    } );
    // POP H (0xe1) L <- (sp); H <- (sp+1); sp <- sp+2
    // 10 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xe1, 
        [this](){
            // pop into l
            this->state.l = this->memory->read(this->state.sp++);
            // pop into h
            this->state.h = this->memory->read(this->state.sp++);
            ++this->state.pc;
            return 10; 
        } 
    } );
    // JPO (0xe2) if PO, PC <- adr:
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xe2, 
        [this](){
            if (!(this->state.isFlag(State8080::P))) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1);
                this->state.pc = jumpAddress; 
            } else {
                this->state.pc += 3;
            }
            return 10;
        } 
    } );
    // XTHL (0xe3) L <-> (SP); H <-> (SP+1)
    // 18 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xe3, 
        [this](){
            uint8_t templ = this->state.l;
            uint8_t temph = this->state.h;
            this->state.l = this->memory->read(this->state.sp);
            this->state.h = this->memory->read(this->state.sp + 1);
            this->memory->write(templ, this->state.sp);
            this->memory->write(temph, this->state.sp + 1);
            ++this->state.pc;
            return 18; 
        } 
    } );
    // CPO (0xe4) if PO, CALL adr
    // 17 cycles if call; otherwise 11, 3 bytes
    // no flags
    opcodes.insert( { 0xe4, 
        [this](){
            // read destination address do call actions
            if (!(this->state.isFlag(State8080::P))) {
                return(this->decode(0xcd))(); // CALL (0xcd) 17 cycles 
            } else {
                this->state.pc += 3;
                return 11;
            }
            
        } 
    } );
    // PUSH H (0xe5) (sp-2)<-L; (sp-1)<-H; sp <- sp - 2:
    // 11 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xe5, 
        [this](){
            // push h
            this->memory->write(this->state.h, --this->state.sp);
            // push l
            this->memory->write(this->state.l, --this->state.sp);
            ++this->state.pc;
            return 11; 
        } 
    } );
    // ANI (0xe6) A <- A & data:
    // 7 cycles, 2 bytes
    // Z, S, P, AC ,CY
    opcodes.insert( { 0xe6, 
        [this](){
            this->state.a = 
                this->andWithAccumulator(this->memory->read(this->state.pc +1));

            this->state.pc += 2;
            return 7; 
        } 
    } );
    // RPE (0xe8) if PE, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xe8, 
        [this](){
            if (this->state.isFlag(State8080::P)) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
        } 
    } );
    // JPE (0xea) if PE, PC <- adr
    // 10 cycles, 3 bytes
    // no flags 
    opcodes.insert( { 0xea, 
        [this](){
            if (this->state.isFlag(State8080::P)) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1);
                this->state.pc = jumpAddress; 
            } else {
                this->state.pc += 3;
            }
            return 10;
        } 
    } );
    // XCHG (0xeb) H <-> D; L <-> E:
    // 4 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xeb, 
        [this](){
            uint8_t temp = this->state.h;
            this->state.h = this->state.d;
            this->state.d = temp;

            temp = this->state.l;
            this->state.l = this->state.e;
            this->state.e = temp;

            ++this->state.pc;
            return 4; 
        } 
    } );
    // CPE (0xec) if PE, CALL adr
    // 17 cycles if call; otherwise 11, 3 bytes
    // no flags
    opcodes.insert( { 0xec, 
        [this](){
            // read destination address do call actions
            if (this->state.isFlag(State8080::P)) {
                return (this->decode(0xcd))(); // CALL (0xcd) 17 cycles 
            } else {
                this->state.pc += 3;
                return 11;
            }
        } 
    } );
    // XRI A <- A ^ data:
    // 7 cycles, 2 bytes
    // Z, S, P, CY, AC
    opcodes.insert( { 0xee, 
        [this](){
            this->state.a = 
                this->xorWithAccumulator(this->memory->read(this->state.pc +1));

            this->state.pc += 2;
            return 7; 
        } 
    } );
    // RP (0xf0) if P, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xf0, 
        [this](){
            if (!(this->state.isFlag(State8080::S))) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
        } 
    } );
    // POP PSW (0xf1) flags <- (sp); A <- (sp+1); sp <- sp+2
    // 10 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xf1, 
        [this](){
            this->state.loadFlags(this->memory->read(this->state.sp++));
            this->state.a = this->memory->read(this->state.sp++);

            ++this->state.pc;
            return 10; 
        } 
    } );
    // JP (0xf2) if P=1 PC <- adr
    // 10 cycles
    // no flags
    opcodes.insert( { 0xf2, 
        [this](){
            if (!(this->state.isFlag(State8080::S))) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1);
                this->state.pc = jumpAddress; 
            } else {
                this->state.pc += 3;
            }
            return 10;
        } 
    } );
    // CP (0xf4) if P, CALL adr
    // 17 cycles if call; otherwise 11, 3 bytes
    // no flags
    opcodes.insert( { 0xf4, 
        [this](){
            // read destination address do call actions
            if (!(this->state.isFlag(State8080::S))) {
                return (this->decode(0xcd))(); // CALL (0xcd) 17 cycles 
            } else {
                this->state.pc += 3;
                return 11;
            }
            
        } 
    } );
    // PUSH PSW (0xf5) (sp-2)<-flags; (sp-1)<-A; sp <- sp - 2
    // 11 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xf5, 
        [this](){
            // push a
            this->memory->write(this->state.a, --this->state.sp);
            //push flags
            this->memory->write(this->state.getFlags(), --this->state.sp);
            ++this->state.pc;
            return 11; 
        } 
    } );
    // ORI (0xf6) A <- A | data
    // 7 cycles, 2 bytes
    // Z, S, P, CY, AC
    opcodes.insert( { 0xf6, 
        [this](){
            this->state.a = 
                this->orWithAccumulator(this->memory->read(this->state.pc +1));

            this->state.pc += 2;
            return 7; 
        } 
    } );
    // RM (0xf8) if M, RET
    // 11 cycles if return; otherwise 5, 1 byte
    // no flags
    opcodes.insert( { 0xf8, 
        [this](){
            if (this->state.isFlag(State8080::S)) {
                // RET (0xc9) 10 cycles, add 1
                return (this->decode(0xc9))() + 1;  
            } else {
                ++this->state.pc;
                return 5;
            }
        } 
    } );
    // JM (0xfa) if M, PC <- adr
    // 10 cycles, 3 bytes
    // no flags
    opcodes.insert( { 0xfa, 
        [this](){
            if (this->state.isFlag(State8080::S)) {
                uint16_t jumpAddress = 
                    this->readAddressFromMemory(this->state.pc + 1);
                this->state.pc = jumpAddress; 
            } else {
                this->state.pc += 3;
            }
            return 10;
        } 
    } );
    // EI (0xfb) special
    // 4 cycles, 1 byte
    // no flags
    opcodes.insert( { 0xfb, 
        [this](){
            this->enableInterrupts = true;
            this->state.pc += 1;
            return 4; 
        } 
    } );
    // CM (0xfc) if M, CALL adr
    // 17 cycles if call; otherwise 11, 3 bytes
    // no flags
    opcodes.insert( { 0xfc, 
        [this](){
            // read destination address do call actions
            if (this->state.isFlag(State8080::S)) {
                return (this->decode(0xcd))(); // CALL (0xcd) 17 cycles 
            } else {
                this->state.pc += 3;
                return 11;
            }
            
        } 
    } );
    // CPI (0xfe) A - data:
    // 7 cycles, 2 bytes
    // Z, S, P, CY, AC
    opcodes.insert( { 0xfe, 
        [this](){
            // diregard return value, we only need flags set for CPI
            this->subtractValues(
                this->state.a, // minuend
                this->memory->read(this->state.pc + 1) //subtrahend
            );
            this->state.pc += 2;
            return 7; 
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
uint8_t Emulator8080::decrementValue(uint8_t value) {
    //check aux carry (https://www.reddit.com/r/EmuDev/comments/p8b4ou/8080_decrement_sub_wrappingzero_question/)
    if (((value & 0x0f) - 1) > 0x0f) {
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }
    --value; 
    // set flags based on result of operation
    this->updateZeroFlag(value);
    this->updateSignFlag(value);
    this->updateParityFlag(value);

    this->state.unSetFlag(State8080::AC);
    return value; // return the decremented value
}

// decrement a value, set Z S P AC flags
uint8_t Emulator8080::incrementValue(uint8_t value) {
    //check aux carry (https://www.reddit.com/r/EmuDev/comments/p8b4ou/8080_decrement_sub_wrappingzero_question/)
    if (((value & 0x0f) + 1) > 0x0f) {
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }
    ++value; 
    // set flags based on result of operation
    this->updateZeroFlag(value);
    this->updateSignFlag(value);
    this->updateParityFlag(value);

    this->state.unSetFlag(State8080::AC);
    return value; // return the decremented value
}

// subtract subtrahend from minuend, set Z, S, P, CY, AC flags
// return minuend - subtrahend
uint8_t Emulator8080::subtractValues(
    uint8_t minuend, uint8_t subtrahend, bool withCarry
) {
    // set up carry bit
    uint8_t carryBit = 0;
    if (withCarry && this->state.isFlag(State8080::CY)) carryBit = 1;

    //check aux carry (https://www.reddit.com/r/EmuDev/comments/p8b4ou/8080_decrement_sub_wrappingzero_question/)
    if (((minuend & 0x0f) - (subtrahend & 0x0f) - carryBit) & 0xf0) {
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }
    // do the subtraction upcast to uint16_t in order to capture carry bit
    uint16_t result = minuend - subtrahend - carryBit;
    // get the 1-byte difference
    uint8_t difference = result & 0x00ff;
    // set flags based on result of operation
    this->updateZeroFlag(difference);
    this->updateSignFlag(difference);
    this->updateParityFlag(difference);

    // AC flag only set for addition
    this->state.unSetFlag(State8080::AC);

    // determine state of carry flag
    if (result & 0x0100) { //0b0000'0001'0000'0000 mask
        this->state.setFlag(State8080::CY);
    } else {
        this->state.unSetFlag(State8080::CY);
    }

    return difference;
}


// add a 2-byte addend to HL and store the result in HL
// set the CY flag if necessary
void Emulator8080::doubleAddWithHLIntoHL(uint16_t addend) {
    // upcast to 4-byte to capture carry bit
    uint32_t result = this->getHL() + addend;
    // get the 2-byte sum
    uint16_t sum = result & 0x0000ffff;
    // store sum in HL
    this->state.h = (sum & 0xff00) >> 8;
    this->state.l = sum & 0x00ff;
    // determine state of carry flag
    if (result & 0x00010000) { //0b0000'0000'0000'0001'0000'0000'0000'0000 mask
        this->state.setFlag(State8080::CY);
    } else {
        this->state.unSetFlag(State8080::CY);
    }
}

// return the bitwise and of a value and the accumulator (a register)
// set Z S P CY AC flags
uint8_t Emulator8080::andWithAccumulator(uint8_t value) {
    // http://bitsavers.trailing-edge.com/components/intel/MCS80/9800301D_8080_8085_Assembly_Language_Programming_Manual_May81.pdf
    // page 1-12 for aux carry
    //check aux carry (https://www.reddit.com/r/EmuDev/comments/p8b4ou/8080_decrement_sub_wrappingzero_question/)
    if ((this->state.a & 0b0000'1000) | (value & 0b0000'1000)) {
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }
    uint8_t result = this->state.a & value; 
    //update flags
    this->updateZeroFlag(result);
    this->updateSignFlag(result);
    this->updateParityFlag(result);
    this->state.unSetFlag(State8080::CY);

    return result;
}


// return the sum an addend and the accumulator (a register)
// additionally adding the carry bit
// set Z S P CY AC flags
uint8_t Emulator8080::addWithAccumulator(uint8_t addend, bool withCarry) {
    // do addition upcast to capture carry bit
    uint16_t result = this->state.a + addend;
    if (this->state.isFlag(State8080::CY) && withCarry) ++result;

    uint8_t sum = result & 0x00ff; // extract 1-byte sum

    // update flags
    this->updateZeroFlag(sum);
    this->updateSignFlag(sum);
    this->updateParityFlag(sum);

    // check AC flag since this is an add
    // sum the low nibbles and see if this makes a carry into the high nibble
    uint8_t nibbleSum = (this->state.a & 0x0f) + (addend & 0x0f);
    if (this->state.isFlag(State8080::CY) && withCarry) ++nibbleSum;
    if (0x10 & nibbleSum) { // low nibble sum
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }

    // determine state of carry flag
    // do this last since other operations rely on the state of this flag
    if (result & 0x0100) { //0b0000'0001'0000'0000 mask
        this->state.setFlag(State8080::CY);
    } else {
        this->state.unSetFlag(State8080::CY);
    }

    return sum;
}

// return the bitwise or of a value and the accumulator (a register)
// set Z S P CY AC flags
uint8_t Emulator8080::orWithAccumulator(uint8_t value) {
    this->state.unSetFlag(State8080::CY);
    this->state.unSetFlag(State8080::AC);
    uint8_t result = value | this->state.a;
    this->updateZeroFlag(result);
    this->updateSignFlag(result);
    this->updateParityFlag(result);
    return result;
}

// return the bitwise xor of a value and the accumulator (a register)
// set Z S P CY AC flags
uint8_t Emulator8080::xorWithAccumulator(uint8_t value) {
    this->state.unSetFlag(State8080::CY);
    this->state.unSetFlag(State8080::AC);
    uint8_t result = value ^ this->state.a;
    this->updateZeroFlag(result);
    this->updateSignFlag(result);
    this->updateParityFlag(result);
    return result;
}

