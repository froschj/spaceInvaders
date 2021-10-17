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
        void currentAddress(); // print the current address
        void twoByteOperand(const uint16_t startAddress); //print 2 bytes
        void oneByteOperand(const uint16_t address); //print 1 byte
        void mnemonic(const std::string mnemonic); //print opcode mnemonic
        // print a sequence of words starting from address
        void instructionBytes(const uint16_t address, const int words);
        
        // opcode declarations
        int NOP();      //0x00
        int LXI_B();    //0x01
        int STAX_B();   //0x02
        int INX_B();    //0x03
        int INR_B();    //0x04
        int DCR_B();    //0x05
        int MVI_B();    //0x06
        int RLC();      //0x07
        //  ???           0x08
        int DAD_B();    //0x09
        int LDAX_B();   //0x0a
        int DCX_B();    //0x0b
        int INR_C();    //0x0c
        int DCR_C();    //0x0d
        int MVI_C();    //0x0e
        int RRC();      //0x0f
        //  ???         //0x10
        int LXI_D();    //0x11
        int STAX_D();   //0x12
        int INX_D();    //0x13
        int INR_D();    //0x14
        int DCR_D();    //0x15
        int MVI_D();    //0x16
        int RAL();      //0x17
        //  ???           0x18
        int DAD_D();    //0x19
        int LDAX_D();   //0x1a
        int DCX_D();    //0x1b
        int INR_E();    //0x1c
        int DCR_E();    //0x1d
        int MVI_E();    //0x1e
        int RAR();      //0x1f
        //  ???           0x20
        int LXI_H();    //0x21
        int SHLD();     //0x22
        int INX_H();    //0x23
        int INR_H();    //0x24
        int DCR_H();    //0x25
        int MVI_H();    //0x26
        int DAA();      //0x27
        //  ???           0x28
        int DAD_H();    //0x29
        int LHLD();     //0x2a
        int DCX_H();    //0x2b
        int INR_L();    //0x2c
        int DCR_L();    //0x2d
        int MVI_L();    //0x2e
        int CMA();      //0x2f
        //  ???           0x30
        int LXI_SP();   //0x31
        int STA();      //0x32
        int INX_SP();   //0x33
        int INR_M();    //0x34
        int DCR_M();    //0x35
        int MVI_M();    //0x36
        int STC();      //0x37
        //  ???         //0x38
        int DAD_SP();   //0x39
        int LDA();      //0x3a
        int DCX_SP();   //0x3b
        int INR_A();    //0x3c
        int DCR_A();    //0x3d
        int MVI_A();    //0x3e
        int CMC();      //0x3f
        int MOV_B_B();  //0x40
        int MOV_B_C();  //0x41
        int MOV_B_D();  //0x42
        int MOV_B_E();  //0x43
        int MOV_B_H();  //0x44
        int MOV_B_L();  //0x45
        int MOV_B_M();  //0x46
        int MOV_B_A();  //0x47
        int MOV_C_B();  //0x48
        int MOV_C_C();  //0x49
        int MOV_C_D();  //0x4a
        int MOV_C_E();  //0x4b
        int MOV_C_H();  //0x4c
        int MOV_C_L();  //0x4d
        int MOV_C_M();  //0x4e
        int MOV_C_A();  //0x4f
        int MOV_D_B();  //0x50
        int MOV_D_C();  //0x51
        int MOV_D_D();  //0x52
        int MOV_D_E();  //0x53
        int MOV_D_H();  //0x54
        int MOV_D_L();  //0x55
        int MOV_D_M();  //0x56
        int MOV_D_A();  //0x57
        int MOV_E_B();  //0x58
        int MOV_E_C();  //0x59
        int MOV_E_D();  //0x5a
        int MOV_E_E();  //0x5b
        int MOV_E_H();  //0x5c
        int MOV_E_L();  //0x5d
        int MOV_E_M();  //0x5e
        int MOV_E_A();  //0x5f
        int MOV_H_B();  //0x60
        int MOV_H_C();  //0x61
        int MOV_H_D();  //0x62
        int MOV_H_E();  //0x63
        int MOV_H_H();  //0x64
        int MOV_H_L();  //0x65
        int MOV_H_M();  //0x66
        int MOV_H_A();  //0x67
        int MOV_L_B();  //0x68
        int MOV_L_C();  //0x69
        int MOV_L_D();  //0x6a
        int MOV_L_E();  //0x6b
        int MOV_L_H();  //0x6c
        int MOV_L_L();  //0x6d
        int MOV_L_M();  //0x6e
        int MOV_L_A();  //0x6f
        int MOV_M_B();  //0x70
        int MOV_M_C();  //0x71
        int MOV_M_D();  //0x72
        int MOV_M_E();  //0x73
        int MOV_M_H();  //0x74
        int MOV_M_L();  //0x75
        int HLT();      //0x76
        int MOV_M_A();  //0x77
        int MOV_A_B();  //0x78
        int MOV_A_C();  //0x79
        int MOV_A_D();  //0x7a
        int MOV_A_E();  //0x7b
        int MOV_A_H();  //0x7c
        int MOV_A_L();  //0x7d
        int MOV_A_M();  //0x7e
        int MOV_A_A();  //0x7f
        int ADD_B();    //0x80
        int ADD_C();    //0x81
        int ADD_D();    //0x82
        int ADD_E();    //0x83
        int ADD_H();    //0x84
        int ADD_L();    //0x85
        int ADD_M();    //0x86
        int ADD_A();    //0x87
        int ADC_B();    //0x88
        int ADC_C();    //0x89
        int ADC_D();    //0x8a
        int ADC_E();    //0x8b
        int ADC_H();    //0x8c
        int ADC_L();    //0x8d
        int ADC_M();    //0x8e
        int ADC_A();    //0x8f
        int SUB_B();    //0x90
        int SUB_C();    //0x91
        int SUB_D();    //0x92
        int SUB_E();    //0x93
        int SUB_H();    //0x94
        int SUB_L();    //0x95
        int SUB_M();    //0x96
        int SUB_A();    //0x97
        int SBB_B();    //0x98
        int SBB_C();    //0x99
        int SBB_D();    //0x9a
        int SBB_E();    //0x9b
        int SBB_H();    //0x9c
        int SBB_L();    //0x9d
        int SBB_M();    //0x9e
        int SBB_A();    //0x9f
        int ANA_B();    //0xa0   
        int ANA_C();    //0xa1
        int ANA_D();    //0xa2
        int ANA_E();    //0xa3
        int ANA_H();    //0xa4
        int ANA_L();    //0xa5
        int ANA_M();    //0xa6
        int ANA_A();    //0xa7
        int XRA_B();    //0xa8
        int XRA_C();    //0xa9
        int XRA_D();    //0xaa
        int XRA_E();    //0xab
        int XRA_H();    //0xac
        int XRA_L();    //0xad
        int XRA_M();    //0xae
        int XRA_A();    //0xaf
        int ORA_B();    //0xb0
        int ORA_C();    //0xb1
        int ORA_D();    //0xb2
        int ORA_E();    //0xb3
        int ORA_H();    //0xb4
        int ORA_L();    //0xb5
        int ORA_M();    //0xb6
        int ORA_A();    //0xb7
        int CMP_B();    //0xb8
        int CMP_C();    //0xb9
        int CMP_D();    //0xba
        int CMP_E();    //0xbb
        int CMP_H();    //0xbc
        int CMP_L();    //0xbd
        int CMP_M();    //0xbe
        int CMP_A();    //0xbf
        int RNZ();      //0xc0
        int POP_B();    //0xc1
        int JNZ();      //0xc2
        int JMP();      //0xc3
        int CNZ();      //0xc4
        int PUSH_B();   //0xc5
        int ADI();      //0xc6
        int RST_0();    //0xc7
        int RZ();       //0xc8
        int RET();      //0xc9
        int JZ();       //0xca
        //  ???           0xcb
        int CZ();       //0xcc
        int CALL();     //0xcd
        int ACI();      //0xce
        int RST_1();    //0xcf
        int RNC();      //0xd0
        int POP_D();    //0xd1
        int JNC();      //0xd2
        int OUT();      //0xd3
        int CNC();      //0xd4
        int PUSH_D();   //0xd5
        int SUI();      //0xd6
        int RST_2();    //0xd7
        int RC();       //0xd8
        //  ???           0xd9
        int JC();       //0xda
        int IN();       //0xdb
        int CC();       //0xdc
        //  ???           0xdd
        int SBI();      //0xde
        int RST_3();    //0xdf
        int RPO();      //0xe0
        int POP_H();    //0xe1
        int JPO();      //0xe2
        int XTHL();     //0xe3
        int CPO();      //0xe4
        int PUSH_H();   //0xe5
        int ANI();      //0xe6
        int RST_4();    //0xe7
        int RPE();      //0xe8
        int PCHL();     //0xe9
        int JPE();      //0xea
        int XCHG();     //0xeb
        int CPE();      //0xec
        //  ???           0xed
        int XRI();      //0xee
        int RST_5();    //0xef
        int RP();       //0xf0
        int POP_PSW();  //0xf1
        int JP();       //0xf2
        int DI();       //0xf3
        int CP();       //0xf4
        int PUSH_PSW(); //0xf5
        int ORI();      //0xf6
        int RST_6();    //0xf7
        int RM();       //0xf8
        int SPHL();     //0xf9
        int JM();       //0xfa
        int EI();       //0xfb
        int CM();       //0xfc
        //  ???           0xfd
        int CPI();      //0xfe
        int RST_7();    //0xff
        
};

#endif