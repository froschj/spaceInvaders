

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
    // RRC (0x0f) A = A << 1; bit 0 = prev bit 7; CY = prev bit 7:
    // 4 cycles, 1 bytes
    // CY
    opcodes.insert( { 0x0f, 
        [this](){
            uint16_t temp = this->state.a << 1;
            this->state.a = static_cast<uint8_t>(temp & 0x00ff);
            // wrap the carry bit arround
            this->state.a |= static_cast<uint8_t>((temp >> 8) & 0x0001);
            // set the carry flag
            if (temp & 0x0100) {
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
    // MVI H (0x26) H <- byte 2:
    // 7 cycles, 2 bytes
    // no flags
    opcodes.insert( { 0x26, 
        [this](){ 
            this->state.h = this->memory->read(this->state.pc + 1);
            this->state.pc +=2;
            return 7; 
        } 
    } );
    // DAD H (0x29) HL = HL + HL
    // 10 cycles, 1 byte
    // CY
    opcodes.insert( { 0x29, 
        [this](){ 
            this->doubleAddWithHLIntoHL(this->getHL());
            this->state.pc +=1;
            return 10; 
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

// subtract subtrahend from minuend, set Z, S, P, CY, AC flags
// return minuend - subtrahend
uint8_t Emulator8080::subtractValues(uint8_t minuend, uint8_t subtrahend) {
    //check aux carry (https://www.reddit.com/r/EmuDev/comments/p8b4ou/8080_decrement_sub_wrappingzero_question/)
    if (((minuend & 0x0f) - (subtrahend & 0x0f)) & 0xf0) {
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }
    // do the subtraction upcast to uint16_t in order to capture carry bit
    uint16_t result = minuend - subtrahend;
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
// set Z S P CY AC flags
uint8_t Emulator8080::addWithAccumulator(uint8_t addend) {
    // do addition upcast to capture carry bit
    uint16_t result = this->state.a + addend;

    uint8_t sum = result & 0x00ff; // extract 1-byte sum

    // update flags
    this->updateZeroFlag(sum);
    this->updateSignFlag(sum);
    this->updateParityFlag(sum);

    // check AC flag since this is an add
    // sum the low nibbles and see if this makes a carry into the high nibble
    if (0x10 & ((this->state.a & 0x0f) + (addend & 0x0f))) { // low nibble sum
        this->state.setFlag(State8080::AC);
    } else {
        this->state.unSetFlag(State8080::AC);
    }

    // determine state of carry flag
    if (result & 0x0100) { //0b0000'0001'0000'0000 mask
        this->state.setFlag(State8080::CY);
    } else {
        this->state.unSetFlag(State8080::CY);
    }

    return sum;
}

// return the bitwise xor of a value and the accumulator (a register)
// set Z S P CY AC flags
uint8_t Emulator8080::xorWithAccumulator(uint8_t value) {
    this->state.unSetFlag(State8080::CY);
    this->state.unSetFlag(State8080::AC);
    uint8_t result = value ^ this->state.a;
    this->updateZeroFlag(value);
    this->updateSignFlag(value);
    this->updateParityFlag(value);
    return result;
}

