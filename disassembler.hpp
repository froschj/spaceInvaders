/*
 * Interface for Disassembler "Processor"
 */

#ifndef DISASSEMBLER_HPP
#define DISASSEMBLER_HPP

#include "processor.hpp"
#include <cstdint>
#include "memory.hpp"
#include <ostream>
#include <map>
#include <iostream>
#include <exception>
#include <string>

/*
 * State for the 8080 Disassembler. This only needs a pc register, since it 
 * will print to an ostream instead of execting instructions
 */
struct DisassemblerState8080 : State {
    private:
        virtual struct DisassemblerState8080* doClone() const {
            struct DisassemblerState8080 *temp = 
                new struct DisassemblerState8080;
                temp->pc = this->pc;
                return temp;
        } 
    public:
        std::unique_ptr<struct DisassemblerState8080> clone() const {
            return std::unique_ptr<struct DisassemblerState8080>(doClone());
        }
        uint16_t pc;
};


/*
 * A meaningful exception to throw if there is an out-of-bounds 
 * memory read by the "processor"
 */
class MemoryReadError : public std::exception {
    private:
        std::string msg;
    public:
        MemoryReadError(const std::string& address) : 
                msg(std::string("Invalid read at address: ") + address){}
        virtual const char *what() const throw() {
            return msg.c_str();
        }
};

/*
 * A meaningful exception to throw if the "processor" encounters an
 * unknown or unimplemented opcode
 */
class UnimplememntedInstructionError : public std::exception {
    private:
        std::string msg;
    public:
        UnimplememntedInstructionError(
                const std::string& address,
                const std::string& opcode 
        ) : 
                msg(
                    std::string("Invalid opcode ") 
                    + opcode + 
                    std::string(" at address: ") 
                    + address){}
        virtual const char *what() const throw() {
            return msg.c_str();
        }
};

/*
 * Derive the 8080 Disassembler from the generic processor class.
 * The template parameters can be defined here since thay are known
 */
class Disassembler8080 : 
        public Processor<struct DisassemblerState8080, Memory> {
    public:
        // default to cout as the output device
        Disassembler8080(std::ostream &os = std::cout);
        // construct with a memory attached
        Disassembler8080(Memory *memoryDevice, std::ostream &os = std::cout);
        ~Disassembler8080();
        int step() override; // "execute" an instruction
        void reset(uint16_t address = 0x0000); // put the pc at an address
    private:
        // for readability
        typedef int (Disassembler8080::*OpcodePtr)(void);

        uint8_t fetch(uint16_t address); // fetch instruction at address
        OpcodePtr decode(uint8_t); // decode an opcode and get its execution
        std::map<uint8_t, OpcodePtr> opcodes; // hold opcode lookup table
        std::ostream& outputDevice; // store the selected output
        void buildMap(); // populate the lookup table
        
        // opcode declarations
        int NOP();
};

#endif