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
    std::vector<uint8_t> rom(romLength);
    romFile.read(reinterpret_cast<char*>(rom.data()), romLength);
    if (romFile.fail()){
        std::cerr << "Error reading file." << std::endl;
        return 1;
    }
    romFile.close();

    if (args->isHexDumpMode){
        // print the hex dump
        char printable[DISPLAY_WIDTH + 1];
        printable[DISPLAY_WIDTH] = '\0';
        int printableIndex = 0;
        std::cout << std::hex << std::setfill('0');
        for (auto it = rom.begin(); it !=rom.end(); ++it){
            if ((it - rom.begin()) % DISPLAY_WIDTH == 0) {
                if (it != rom.begin()){
                    std::cout << printable;
                    std::cout << std::endl;
                    printableIndex = 0;
                }
                std::cout << std::setw(4) << it - rom.begin() << " ";
            }
            std::cout << std::setw(2) << static_cast<int>(*it) << " ";
            if (*it < 32 || *it > 126){
                printable[printableIndex] = '.';
            } else {
                printable[printableIndex] = *it;
            }
            ++printableIndex;
        }
        std::cout << printable;
        std::cout << std::endl;
    } else {
        // do the disassembly
        std::cout << "Disassembly not yet implemented." << std::endl;
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