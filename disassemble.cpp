/*
 *  Driver program for running an 8080 Disassembler from the command line
 */

#ifdef _WIN32
#define TCLAP_NAMESTARTSTRING "~~"
#define TCLAP_FLAGSTARTSTRING "/"
#endif

#include "tclap/CmdLine.h"
#include <string>
#include <memory>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <iomanip>
#include "memory.hpp"
#include "processor.hpp"
#include "disassembler.hpp"
#include <vector>
#include <string>
#include <bitset>
#include "emulator.hpp"
#include <chrono>



#define DISASSEMBLE_VERSION

const int DISPLAY_WIDTH = 16;

/*
 * Struct holding the arguments retrieved from the command line
 */
struct disassembleArguments {
    std::string commandName;
    std::string romFileName;
    bool isCpmMode;
};

/*
 * Invoke parser from TCLAP library to process the command line
 * Returns nullptr on failure.
 */
std::unique_ptr <struct disassembleArguments> parseArguments(
    int argumentCount, char *argumentVector[]
);

int main(int argc, char *argv[]) {
	std::ios_base::sync_with_stdio(false);
    // send the argment variables to the TCLAP parser
    std::unique_ptr<struct disassembleArguments> args = 
        parseArguments(argc, argv);

    // if we failed to parse
    if(!args){
        return 1;
    }

    // set the address to load the code at
    uint16_t startAddress = 0x0000;
    if (args->isCpmMode) startAddress = 0x0100;

    // open the file
    std::ifstream romFile;
    romFile.open(args->romFileName, std::ios::binary);
    if (romFile.fail()){
        std::cerr << "Could not open file: " << args->romFileName << '\n';
        return 1;
    }

    // file opened successfully, read it into memory
    // get the length
    romFile.seekg(0, romFile.end);
    int romLength = romFile.tellg();
    romFile.seekg(0, romFile.beg);
    if (romLength + startAddress > 0x10000) {
        std::cerr << "File too long." << '\n';
        return 1;
    }

    // create a buffer and read into it
    std::unique_ptr<std::vector<uint8_t>> tempROM = 
        std::make_unique<std::vector<uint8_t>> (0x10000);
    romFile.read(
        reinterpret_cast<char*>(tempROM->data() + startAddress), 
        romLength
    );
    if (romFile.fail()){
        std::cerr << "Error reading file." << '\n';
        return 1;
    }
    romFile.close();

    

    // move buffer into Memory object
    Memory rom(std::move(tempROM));

    if (args->commandName == "hexdump"){
        // print the hex dump
        char printable[DISPLAY_WIDTH + 1];
        printable[DISPLAY_WIDTH] = '\0';
        int printableIndex = 0;
        std::cout << std::hex << std::setfill('0');
        for (int i = startAddress; i < (romLength + startAddress); ++i){
            if (i % DISPLAY_WIDTH == 0) {
                if (i != 0){
                    std::cout << printable;
                    std::cout << '\n';
                    printableIndex = 0;
                }
                std::cout << std::setw(4) << i << " ";
            }
            std::cout << std::setw(2) << static_cast<int>(rom.read(i)) << " ";
            if (rom.read(i) < 32 || rom.read(i) > 126){
                printable[printableIndex] = '.';
            } else {
                printable[printableIndex] = rom.read(i);
            }
            ++printableIndex;
        }
        std::cout << printable;
        std::cout << '\n';
    } else if (args->commandName == "disassemble") {
        // do the disassembly
        Disassembler8080 debug8080(&rom);
        debug8080.reset(startAddress); //start at address 0x0000
        try {
            // processor will disassemble until the end of memory
            while (debug8080.getState()->pc < (startAddress + romLength)) {
                debug8080.step();
            }
        } catch (const std::exception& e) {
            // processor throws excptions on illegal memory read
            // and on unknown opcode
            std::cerr << e.what() << '\n';
            return 1;
        }

    } else if ((args->commandName == "debug") || (args->commandName == "run")) {
        Disassembler8080 disassembler(&rom);
        Emulator8080 emulator(&rom);
        if (args->commandName == "debug") disassembler.reset(startAddress);
        emulator.reset(startAddress);

        // set up memory for cpu emulation
        // and emulate BDOS calls
        if (args->isCpmMode) {
            rom.write(0xc3, 0x0005); //JMP $e400
            rom.write(0x00, 0x0006);
            rom.write(0xe4, 0x0007);

            rom.write(0xf5, 0xe400); //PUSH PSW
            rom.write(0x79, 0xe401); //MOV A,C
            rom.write(0xd3, 0xe402); //OUT $ff
            rom.write(0xff, 0xe403);
            rom.write(0xf1, 0xe404); //POP PSW
            rom.write(0xc9, 0xe405); //RET

            auto outputPort = [&emulator, &rom](uint8_t port, uint8_t value){
                if (port == 0xff) {
                    if (value == 9) {
                        // C_WRITESTR system call
                        auto cpuRegisters = emulator.getState();
                        uint16_t stringOffset = (cpuRegisters->d << 8) 
                            + cpuRegisters->e;
                        //stringOffset += 3; // skip prefix
                        //std::cout << '\n'; // prefix is a newline
                        while (
                            static_cast<char>(rom.read(stringOffset)) != '$'
                        ) {
                            std::cout << static_cast<char>(
                                rom.read(stringOffset)
                            );
                            ++stringOffset;
                        }
                    } else if (value == 2) {
                        // C_WRITE system call
                        auto cpuRegisters = emulator.getState();
                        std::cout << static_cast<char>(cpuRegisters->e);
                    }
                }
                return;
            };

            emulator.connectOutput(outputPort);
        } else {
            emulator.connectOutput([](uint8_t port, uint8_t value){return;});
            
        }
        emulator.connectInput([](uint8_t port){ return 0xff; });

        unsigned long long cycles = 0;
        unsigned long long instructions = 0;
        auto startTime = std::chrono::high_resolution_clock::now();

        try {
            std::unique_ptr<struct State8080> state = nullptr;
            bool finished = false;
            while (!finished) {
                cycles += emulator.step();
                ++instructions;
                if (args->commandName == "debug") {
                    disassembler.step();
                    state = emulator.getState();
                    disassembler.reset(state->pc);
                    std::cout << "Cycles: " << std::dec << cycles << '\n';
                    std::cout << std::right << std::hex << std::setfill('0');
                    std::cout << "A: 0x" << std::setw(2) 
                        << static_cast<int>(state->a) << " ";
                    std::cout << "B: 0x" << std::setw(2) 
                        << static_cast<int>(state->b) << " ";
                    std::cout << "C: 0x" << std::setw(2) 
                        << static_cast<int>(state->c) << " ";
                    std::cout << "D: 0x" << std::setw(2)  
                        << static_cast<int>(state->d) << " ";
                    std::cout << "E: 0x" << std::setw(2)  
                        << static_cast<int>(state->e) << " ";
                    std::cout << "H: 0x" << std::setw(2)  
                        << static_cast<int>(state->h) << " ";
                    std::cout << "L: 0x" << std::setw(2) 
                        << static_cast<int>(state->l) << " ";
                    std::cout << "SP: 0x" << std::setw(4) 
                        << static_cast<int>(state->sp) << " ";
                    std::cout << "PC: 0x" << std::setw(4) 
                        << static_cast<int>(state->pc) << " ";
                    std::cout << "Flags: 0b" << std::setw(8)
                        << std::bitset<8>(static_cast<int>(state->getFlags()));
                    std::cout << '\n';
                }
                if (args->isCpmMode && (emulator.getState()->pc == 0)) {
                    finished = true;
                } 
            } 
        auto stopTime = std::chrono::high_resolution_clock::now();
        std::chrono::duration<long long, std::nano> runTime = 
            stopTime - startTime;
        double runSeconds = static_cast<double>(runTime.count()) * 1.e-9;
        std::cout << std::endl;
        std::cout << "Ran " << instructions << " instructions"; 
        std::cout << " in " << runSeconds << " seconds."<<std::endl;
        std::cout << "Used " << cycles << " cycles." << std::endl;
        double megahertz = (static_cast<double>(cycles) / runSeconds) / 1.e6;
        std::cout << "Approximate clock speed: " << megahertz;
        std::cout << " MHz." << std::endl;
        } catch (const std::exception& e) {
            // processor throws excptions on illegal memory read
            // and on unknown opcode
            std::cerr << e.what() << '\n';
            return 1;
        }
    }
    
    
    std::cout << '\n';
    return 0;
}


/*
 * Invoke parser from TCLAP library to process the command line
 * Returns nullptr on failure.
 */
std::unique_ptr <struct disassembleArguments> parseArguments(
    int argumentCount, char *argumentVector[]
) {
    std::string romFileName;
    std::string commmandName;
    bool isCpm;
    try {
        // TCLAP Parser
        TCLAP::CmdLine cmd(
            "8080 Machine Language disassembler", 
            ' ', 
            "0.1",
            true
        );

        // define a required argument for "commands"
        std::vector<std::string> commands;
        commands.push_back("hexdump");
        commands.push_back("disassemble");
        commands.push_back("debug");
        commands.push_back("run");
        TCLAP::ValuesConstraint<std::string> commandValues(commands);
        TCLAP::UnlabeledValueArg<std::string> commandArg(
            "command",
            "name of command to run",
            true,
            "hexdump",
            &commandValues
        );
        cmd.add(commandArg);
        
        // Argument for file name to load ROM from
        TCLAP::UnlabeledValueArg<std::string> romFileNameArg(
            "fileName", 
            "binary file to load ROM from", 
            true,
            "",
            "string"
        );
        cmd.add(romFileNameArg);

        // switch to turn on cp/m emulation
        TCLAP::SwitchArg cpm(
            "c",
            "cpm",
            "activates cp/m system call handling",
            false
        );
        cmd.add(cpm);

        // Run the parser and extract the values
        cmd.parse(argumentCount, argumentVector);
        romFileName = romFileNameArg.getValue();
        commmandName = commandArg.getValue();
        isCpm = cpm.getValue();
    } 
    catch (TCLAP::ArgException &e){ 
        // if something went wrong, print an error message and return nullptr
        std::cerr << "error: " 
            << e.error() 
            << " for arg " 
            << e.argId() 
            << '\n'; 
        return std::unique_ptr<struct disassembleArguments>(nullptr);
    }

    // build return struct and return a pointer to it on the heap
    std::unique_ptr<struct disassembleArguments> args = 
        std::make_unique<struct disassembleArguments>();
    args->romFileName = romFileName;
    args->commandName = commmandName;
    args->isCpmMode = isCpm;
    return args;
}