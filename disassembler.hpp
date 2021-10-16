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

class MemoryReadError : std::exception {
    private:
        std::string msg;
    public:
        MemoryReadError(const std::string& address) : 
                msg(std::string("Invalid read at address: ") + address){}
        virtual const char *what() const throw() {
            return msg.c_str();
        }
};

class UnimplememntedInstructionError : std::exception {
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

class Disassembler8080 : 
        public Processor<struct DisassemblerState8080, Memory> {
    public:
        Disassembler8080(std::ostream &os = std::cout);
        Disassembler8080(Memory *memoryDevice, std::ostream &os = std::cout);
        ~Disassembler8080();
        void step() override;
        void reset(uint16_t address = 0x0000);
    private:
        typedef int (*OpcodePtr)(void);
        uint8_t fetch(uint16_t address); //fetch instruction at address
        OpcodePtr decode(uint8_t); //decode an opcode and get its execution
        std::map<uint8_t, OpcodePtr> opcodes;
        std::ostream& outputDevice;
        void buildMap();
};

#endif