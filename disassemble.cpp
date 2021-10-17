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
    bool isHexDumpMode;
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
        std::make_unique<std::vector<uint8_t>> (romLength);
    romFile.read(reinterpret_cast<char*>(tempROM->data()), romLength);
    if (romFile.fail()){
        std::cerr << "Error reading file." << std::endl;
        return 1;
    }
    romFile.close();

    // move buffer into Memory object
    Memory rom(std::move(tempROM));

    if (args->isHexDumpMode){
        // print the hex dump
        char printable[DISPLAY_WIDTH + 1];
        printable[DISPLAY_WIDTH] = '\0';
        int printableIndex = 0;
        std::cout << std::hex << std::setfill('0');
        for (int i = rom.getLowAddress(); i <= rom.getHighAddress(); ++i){
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
    } else {
        // do the disassembly
        Disassembler8080 debug8080(&rom);
        debug8080.reset(0x0000); //start at address 0x0000
        try {
            // processor will disassemble until the end of memory
            while (debug8080.getState()->pc <= rom.getHighAddress()) {
                debug8080.step();
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
    bool isHexDumpMode;

    try {
        // TCLAP Parser
        TCLAP::CmdLine cmd(
            "8080 Machine Language disassembler", 
            ' ', 
            "0.1",
            true
        );

        // Argument for file name to load ROM from
        TCLAP::UnlabeledValueArg<std::string> romFileNameArg(
            "fileName", 
            "binary file to load ROM from", 
            true,
            "",
            "string"
        );
        cmd.add(romFileNameArg);

        // Argument for setting hex output mode
        TCLAP::SwitchArg binaryListSwitch(
            "l",
            "list",
            "list file in hexadecimal"
        );

        // Run the parser and extract the values
        cmd.add(binaryListSwitch);
        cmd.parse(argumentCount, argumentVector);
        romFileName = romFileNameArg.getValue();
        isHexDumpMode = binaryListSwitch.getValue();
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
    args->isHexDumpMode = isHexDumpMode;
    return args;
}