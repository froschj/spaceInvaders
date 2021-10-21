/*
 *  Driver program for running an 8080 Disassembler from the command line
 */


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
#include "emulator.hpp"

#ifdef WINDOWS
#define TCLAP_NAMESTARTSTRING "~~"
#define TCLAP_FLAGSTARTSTRING "/"
#endif

#define DISASSEMBLE_VERSION

const int DISPLAY_WIDTH = 16;

/*
 * Struct holding the arguments retrieved from the command line
 */
struct disassembleArguments {
    std::string commandName;
    std::string romFileName;
};

/*
 * Invoke parser from TCLAP library to process the command line
 * Returns nullptr on failure.
 */
std::unique_ptr <struct disassembleArguments> parseArguments(
    int argumentCount, char *argumentVector[]
);

int main(int argc, char *argv[]) {
    // send the argment variables to the TCLAP parser
    std::unique_ptr<struct disassembleArguments> args = 
        parseArguments(argc, argv);

    // if we failed to parse
    if(!args){
        return 1;
    }

    // open the file
    std::ifstream romFile;
    romFile.open(args->romFileName, std::ios::binary);
    if (romFile.fail()){
        std::cerr << "Could not open file: " << args->romFileName << std::endl;
        return 1;
    }

    // file opened successfully, read it into memory
    // get the length
    romFile.seekg(0, romFile.end);
    int romLength = romFile.tellg();
    romFile.seekg(0, romFile.beg);

    // create a buffer and read into it
    std::unique_ptr<std::vector<uint8_t>> tempROM = 
        std::make_unique<std::vector<uint8_t>> (0x2400);
    romFile.read(reinterpret_cast<char*>(tempROM->data()), romLength);
    if (romFile.fail()){
        std::cerr << "Error reading file." << std::endl;
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
        for (int i = rom.getLowAddress(); i < romLength; ++i){
            if (i % DISPLAY_WIDTH == 0) {
                if (i != 0){
                    std::cout << printable;
                    std::cout << std::endl;
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
        std::cout << std::endl;
    } else if (args->commandName == "disassemble") {
        // do the disassembly
        Disassembler8080 debug8080(&rom);
        debug8080.reset(0x0000); //start at address 0x0000
        try {
            // processor will disassemble until the end of memory
            while (debug8080.getState()->pc < romLength) {
                debug8080.step();
            }
        } catch (const std::exception& e) {
            // processor throws excptions on illegal memory read
            // and on unknown opcode
            std::cerr << e.what() << std::endl;
            return 1;
        }

    } else if (args->commandName == "debug") {
        Disassembler8080 disassembler(&rom);
        Emulator8080 emulator(&rom);
        disassembler.reset(0x0000);
        emulator.reset(0x0000);
        try {
            int cycles = 0;
            std::unique_ptr<struct State8080> state = nullptr;
            while (emulator.getState()->pc < romLength) {
                disassembler.step();
                int cycles = emulator.step();
                state = emulator.getState();
                disassembler.reset(state->pc);
                std::ios saveFormat(nullptr);
                saveFormat.copyfmt(std::cout);
                std::cout << "Cycles: " << cycles << std::endl;
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
                std::cout << std::endl;
                std::cout.copyfmt(saveFormat);
            }
        } catch (const std::exception& e) {
            // processor throws excptions on illegal memory read
            // and on unknown opcode
            std::cerr << e.what() << std::endl;
            return 1;
        }
    }

    //std::cout << "Filename: " << args->romFileName << std::endl;
    //std::cout << "Binary Mode? " << args->isHexDumpMode << std::endl;
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

        // Run the parser and extract the values
        cmd.parse(argumentCount, argumentVector);
        romFileName = romFileNameArg.getValue();
        commmandName = commandArg.getValue();
    } 
    catch (TCLAP::ArgException &e){ 
        // if something went wrong, print an error message and return nullptr
        std::cerr << "error: " 
            << e.error() 
            << " for arg " 
            << e.argId() 
            << std::endl; 
        return std::unique_ptr<struct disassembleArguments>(nullptr);
    }

    // build return struct and return a pointer to it on the heap
    std::unique_ptr<struct disassembleArguments> args = 
        std::make_unique<struct disassembleArguments>();
    args->romFileName = romFileName;
    args->commandName = commmandName;
    return args;
}