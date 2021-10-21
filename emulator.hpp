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
            0b1000'0000,    // Z
            0b0100'0000,    // S
            0b0010'0000,    // P
            0b0001'0000,    // CY
            0b0000'1000     // AC
        };
        uint8_t flagsRegister = 0;
    public:
        enum flag {Z,S,P,CY,AC};
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

        uint16_t readAddress(uint16_t atAddress);
        void moveImmediateData(uint8_t &destination, uint8_t data);
        uint16_t getHL();
                
        // catchall for illegal opcodes (probably strings/values in code)
        // int illegal();  //0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38, 
                        //0xcb, 0xd9, 0xdd, 0xed, 0xfd
     
};

#endif