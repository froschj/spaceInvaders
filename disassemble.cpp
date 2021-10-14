/*
 *  Driver program for running an 8080 Disassembler from the command line
 */


#include "tclap/CmdLine.h"
#include <string>
#include <memory>
#include <iostream>

#ifdef WINDOWS
#define TCLAP_NAMESTARTSTRING "~~"
#define TCLAP_FLAGSTARTSTRING "/"
#endif

#define DISASSEMBLE_VERSION

struct disassembleArguments {
    bool isHexDumpMode;
    std::string romFileName;
};

std::unique_ptr <struct disassembleArguments> parseArguments(
    int argumentCount, char *argumentVector[]
);

int main(int argc, char *argv[]) {
    std::unique_ptr<struct disassembleArguments> args = 
        parseArguments(argc, argv);
    if(!args){
        return 1;
    }
    std::cout << "Filename: " << args->romFileName << std::endl;
    std::cout << "Binary Mode? " << args->isHexDumpMode << std::endl;
    return 0;
}

std::unique_ptr <struct disassembleArguments> parseArguments(
    int argumentCount, char *argumentVector[]
) {
    std::string romFileName;
    bool isHexDumpMode;
    try {
        TCLAP::CmdLine cmd(
            "8080 Machine Language disassembler", 
            ' ', 
            "0.1",
            true
        );
        TCLAP::UnlabeledValueArg<std::string> romFileNameArg(
            "fileName", 
            "binary file to load ROM from", 
            true,
            "",
            "string"
        );
        cmd.add(romFileNameArg);
        TCLAP::SwitchArg binaryListSwitch(
            "l",
            "list",
            "list file in hexadecimal"
        );
        cmd.add(binaryListSwitch);
        cmd.parse(argumentCount, argumentVector);
        romFileName = romFileNameArg.getValue();
        isHexDumpMode = binaryListSwitch.getValue();
    } 
    catch (TCLAP::ArgException &e){ 
        std::cerr << "error: " 
            << e.error() 
            << " for arg " 
            << e.argId() 
            << std::endl; 
        return std::unique_ptr<struct disassembleArguments>(nullptr);
    }
    std::unique_ptr<struct disassembleArguments> args = 
        std::make_unique<struct disassembleArguments>();
    args->romFileName = romFileName;
    args->isHexDumpMode = isHexDumpMode;
    return args;
}