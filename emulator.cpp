/*
template<class stateType, class memoryType>
class Processor {
    public:
        // execute an instruction, return # of cycles
*        virtual int step() = 0; 
*        virtual ~Processor() {}
*        Processor() {}
*        std::unique_ptr<stateType> getState() const {
            return state.clone();
        } // return a copy of the current state
*        void connectMemory(memoryType *memoryDevice) {
            memory = memoryDevice;
        }; // connect the processor to a memory
    protected:
        stateType state;
        memoryType *memory;
};

class Emulator8080 : 
        public Processor<struct State8080, Memory> {
    public:
        // default constructor
*        Emulator8080();
        // construct with a memory attached
*        Emulator8080(Memory *memoryDevice);
*        ~Emulator8080();
*        int step() override; // "execute" an instruction
*        void reset(uint16_t address = 0x0000); // put the pc at an address
    private:
        // fetch instruction at address
*        uint8_t fetch(uint16_t address);

        // decode an opcode and get its execution
*        std::function<int(void)> decode(uint8_t);

        // hold opcode lookup table
*        std::map<uint8_t, std::function<int(void)>> opcodes; 

*        void buildMap(); // populate the lookup table
                
        // catchall for illegal opcodes (probably strings/values in code)
*        int illegal();  //0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 
                        //0xcb, 0xd9, 0xdd, 0xed, 0xfd
     
};
*/

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
    // NOP (0x00): 4 cycles
    opcodes.insert( { 0x00, 
        [this](){ 
            ++this->state.pc; 
            return 4; 
        } 
    } );
    // JMP (0xc3) PC <= adr: 10 cycles
    opcodes.insert( { 0xc3, 
        [this](){
            uint16_t lsb = this->memory->read(this->state.pc + 1);
            uint16_t msb = this->memory->read(this->state.pc + 2) << 8; 
            this->state.pc = msb + lsb; 
            return 10; 
        } 
    } );
}

/*
int Emulator8080::illegal() {
    std::stringstream badAddress;
    std::stringstream badOpcode;
    badAddress << "$" 
        << std::setw(4) << std::hex << std::setfill('0') 
        << static_cast<int>(state.pc);
    badOpcode << "$" 
        << std::setw(2) << std::hex << std::setfill('0') 
        << static_cast<int>(memory->read(state.pc));
    throw UnimplementedInstructionError(
        badAddress.str(),
        badOpcode.str()
    );
}*/