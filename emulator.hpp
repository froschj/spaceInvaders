/*
 * Interface for 8080 Emulator object
 */

#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "processor.hpp"
#include <cstdint>
#include "memory.hpp"
#include <ostream>
#include <map>
#include <iostream>
#include <string>
#include <functional>


/*
 * State for the 8080 Disassembler. This only needs a pc register, since it 
 * will print to an ostream instead of execting instructions
 */
struct State8080 : State {
    private:
        virtual struct State8080* doClone() const {
            struct State8080*temp = new struct State8080;
            temp->a = this->a;
            temp->b = this->b;
            temp->c = this->c;
            temp->d = this->d;
            temp->e = this->e;
            temp->h = this->h;
            temp->l = this->l;
            temp->sp = this->sp;
            temp->pc = this->pc;
            temp->flagsRegister = this->flagsRegister;
            return temp;
        }
        const uint8_t flagMasks[5] = {
            0b1000'0000,    // S
            0b0100'0000,    // Z
            0b0001'0000,    // AC
            0b0000'0100,    // P
            0b0000'0001     // CY
        };
        uint8_t flagsRegister = 0b0000'0010;
    public:
        enum flag {S,Z,AC,P,CY};
        std::unique_ptr<struct State8080> clone() const {
            return std::unique_ptr<struct State8080>(doClone());
        }
        uint8_t     a;    
        uint8_t     b;    
        uint8_t     c;    
        uint8_t     d;    
        uint8_t     e;    
        uint8_t     h;    
        uint8_t     l;    
        uint16_t    sp;
        uint16_t    pc;
        uint8_t     getFlags() { return flagsRegister; }
        void        loadFlags(uint8_t flagByte) {
            flagsRegister = flagByte;
            // make sure constant bits are correct
            flagsRegister &= 0b1101'0111;
            flagsRegister |= 0b0000'0010;
        }
        bool isFlag(State8080::flag whichFlag) {
            return static_cast<bool>(flagsRegister & flagMasks[whichFlag]);
        }
        void setFlag(State8080::flag whichFlag) {
            flagsRegister |= flagMasks[whichFlag];
        }
        void unSetFlag(State8080::flag whichFlag) {
            flagsRegister &= ~flagMasks[whichFlag];
        }
        void complementFlag(State8080::flag whichFlag){
            flagsRegister ^= flagMasks[whichFlag];
        }
};

/*
 * Derive the 8080 Disassembler from the generic processor class.
 * The template parameters can be defined here since thay are known
 */
class Emulator8080 : 
        public Processor<struct State8080, Memory> {
    public:
        // default constructor
        Emulator8080();
        // construct with a memory attached
        Emulator8080(Memory *memoryDevice);
        ~Emulator8080();
        int step() override; // "execute" an instruction
        void reset(uint16_t address = 0x0000); // put the pc at an address
    private:
        // fetch instruction at address
        uint8_t fetch(uint16_t address);

        // decode an opcode and get its execution
        std::function<int(void)> decode(uint8_t word);

        // hold opcode lookup table
        std::map<uint8_t, std::function<int(void)>> opcodes; 

        void buildMap(); // populate the lookup table

        uint16_t readAddressFromMemory(uint16_t atAddress);
        void moveImmediateData(uint8_t &destination, uint8_t data);
        uint16_t getBC();
        uint16_t getDE();
        uint16_t getHL();

        void updateZeroFlag(uint8_t value);
        void updateSignFlag(uint8_t value);
        void updateParityFlag(uint8_t value);

        uint8_t decrementValue(uint8_t value);
        uint8_t subtractValues(uint8_t minuend, uint8_t subtrahend);
        uint8_t addWithAccumulator(uint8_t addend);
        void doubleAddWithHLIntoHL(uint16_t addend);
        uint8_t andWithAccumulator(uint8_t value);

        void callAddress(uint16_t address);
                
        // catchall for illegal opcodes (probably strings/values in code)
        // int illegal();  //0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 
                        //0xcb, 0xd9, 0xdd, 0xed, 0xfd
     
};

#endif