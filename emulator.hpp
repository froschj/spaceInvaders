/*
 * Interface for 8080 Emulator object
 */

#ifndef EMULATOR_HPP
#define EMULATOR_HPP

#include "processor.hpp"
#include <cstdint>
#include "memory.hpp"
#include <ostream>
#include <array>
#include <iostream>
#include <string>
#include <functional>
#include <initializer_list>

/*
 * State for the 8080 Emulator. 
 * Internally maintains the registers of an 8080 processor.
 * 
 * Public Interface:
 * a, b, c, d, e, h, l, pc, sp, are all publicly accessible data members.
 * these represent registers and are uint8_t.
 * 
 * State8080::flag defines symbols to represent the different flags:
 * S,Z,AC,P,CY [Sign, Zero, Auxiliary Carry, Parity, CarrY]
 * 
 * getFlags() returns a uint8_t representing the current flags state
 * loadFlags(uint8_t) replaces the flags byte with a supplient uint8_t
 * isFlag(State8080::flag) tests a flag (true=set, false=unset)
 * setFlag(State8080::flag) sets a flag to 1/true
 * unSetFlag(State8080::flag) unsets a flag to 0/false
 * complementFlag(State8080::flag) flips the value of a flag
 * 
 * clone() reurns a unique_ptr to a copy of the current state
 */
struct State8080 : State {
    private:
        virtual struct State8080* doClone() const {
            // a allocate a new State8080 on the heap
            // this is a private method to allow States to be polymorphic
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
        // bitmasks for teh different flags based on their storage in a byte
        const uint8_t flagMasks[5] = {
            0b1000'0000,    // S
            0b0100'0000,    // Z
            0b0001'0000,    // AC
            0b0000'0100,    // P
            0b0000'0001     // CY
        };

        // bit [5] is always 0; bit [1] is always 1
        uint8_t flagsRegister = 0b0000'0010;
    public:
        // meaningful names to work with flags
        enum flag {S,Z,AC,P,CY};
        // public clone interface, converts the private raw pointer into 
        // a unique_ptr, transferring ownership of the copy to the caller
        std::unique_ptr<struct State8080> clone() const {
            return std::unique_ptr<struct State8080>(doClone());
        }

        // cpu registers
        uint8_t     a;    
        uint8_t     b;    
        uint8_t     c;    
        uint8_t     d;    
        uint8_t     e;    
        uint8_t     h;    
        uint8_t     l;    
        uint16_t    sp;
        uint16_t    pc;

        // access the flags
        uint8_t     getFlags() { return flagsRegister; }

        // restore flags from a byte
		void        loadFlags(uint8_t flagByte) {
			flagsRegister = flagByte;
			// make sure constant bits are correct
			// bit [5] is always 0; bit [1] is always 1
			flagsRegister &= 0b1101'0111;
			flagsRegister |= 0b0000'0010;
		}

        // load a state
        void loadState(std::unique_ptr<struct State8080> newState) {
            this->a = newState->a;
            this->b = newState->b;
            this->c = newState->c;
            this->d = newState->d;
            this->e = newState->e;
            this->h = newState->h;
            this->l = newState->l;
            this->sp = newState->sp;
            this->pc = newState->pc;
            this->loadFlags(newState->getFlags());
        }
  
        // return the state of a flag
        bool isFlag(State8080::flag whichFlag) {
            return static_cast<bool>(flagsRegister & flagMasks[whichFlag]);
        }
        // flag is true
        void setFlag(State8080::flag whichFlag) {
            flagsRegister |= flagMasks[whichFlag];
        }
        // flag is false
        void unSetFlag(State8080::flag whichFlag) {
            flagsRegister &= ~flagMasks[whichFlag];
        }
        // flag is !flag
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

		inline bool isInterruptEnable() { return enableInterrupts; }

        // connectMemory(Memory*) provided by paretnt class

        // connect a callback for the OUT instruction
        // first argument is port address, second argument is value
        void connectOutput(std::function<void(uint8_t,uint8_t)> outputFunction);

        // connect a callback for the IN instruction
        // function returns value, argument in port address
        void connectInput(std::function<uint8_t(uint8_t)> inputFunction);

        // request an interrupt. Accepts a one-byte 8080 opcode
        // returns the number of CPU clock cycles to process the interrupt
        int requestInterrupt(uint8_t opcode);
				
		std::unique_ptr<class Snapshot> TakeSnapshot();
		void LoadSnapshot(std::unique_ptr<class Snapshot> snapshot);
		
    private:
        // fetch instruction at address
        uint8_t fetch(uint16_t address);

        // decode an opcode and get its execution
        std::function<int(void)> decode(uint8_t word);

        // hold opcode lookup table
        //std::map<uint8_t, std::function<int(void)>> opcodes; 
        std::array<std::function<int(void)>, 0x100> opcodes;

        void buildMap(); // populate the lookup table

        // hold the callback for OUT instruction
        // arguments are port, value
        std::function<void(uint8_t,uint8_t)> outputCallback;

        // hold the callback for IN instruction
        // argument is port, return value is value
        std::function<uint8_t(uint8_t)> inputCallback;

        // read 2 bytes from memory and convert to address
        uint16_t readAddressFromMemory(uint16_t atAddress);

        // get value from register pairs
        uint16_t getBC();
        uint16_t getDE();
        uint16_t getHL();

        // flag calculators
        void updateZeroFlag(uint8_t value);
        void updateSignFlag(uint8_t value);
        void updateParityFlag(uint8_t value);

        // logical and arithmetic helpers
        uint8_t incrementValue(uint8_t value);
        uint8_t decrementValue(uint8_t value);
        uint8_t subtractValues(
            uint8_t minuend, uint8_t subtrahend, bool withCarry = false
        );
        uint8_t addWithAccumulator(uint8_t addend, bool withCarry = false);
        //uint8_t addWithCarryAccumulator(uint8_t addend);
        void doubleAddWithHLIntoHL(uint16_t addend);
        uint8_t andWithAccumulator(uint8_t value);
        uint8_t xorWithAccumulator(uint8_t value);
        uint8_t orWithAccumulator(uint8_t value);


        // encapusulate call procedures
        void callAddress(uint16_t address, bool isReset = false);

        // processor status
        bool enableInterrupts;
        bool halted;

        // trigger an interrupt
        // accepts an initializer list of bytes representing an 8080 instruction
        // only one-byte instructions currently implemented
        // returns # of cpu clock cycles to handle interrupt
        int processInterrupt(std::initializer_list<uint8_t> instructionBytes);

        int nop();
        int jmp();
        int ret();
        int call();
     
};

/*
 * A meaningful exception to throw if the "processor" encounters an
 * interrupt that cannot be processed
 */
class UnimplementedInterruptError : public std::exception {
    private:
        std::string msg;
    public:
        UnimplementedInterruptError(
                const std::string& opcode 
        ) : 
                msg(
                    std::string("Invalid interrupt instruction ") 
                    + opcode){}
        virtual const char *what() const throw() {
            return msg.c_str();
        }
};

#endif