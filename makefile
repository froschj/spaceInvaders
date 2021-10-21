emulate8080:	disassemble.o memory.o disassembler.o
	g++ disassemble.o memory.o disassembler.o -o emulate8080

disassemble.o:	disassemble.cpp
	g++ -c disassemble.cpp -I./ -std=c++17 

memory.o:		memory.cpp
	g++ -c memory.cpp -std=c++17 

disassembler.o:	disassembler.cpp
	g++ -c disassembler.cpp -std=c++17 
	