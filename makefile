disassemble:	disassemble.o memory.o disassembler.o
	g++ disassemble.o memory.o -o disassemble

disassemble.o:	disassemble.cpp
	g++ -c disassemble.cpp -I./ -std=c++17 

memory.o:		memory.cpp
	g++ -c memory.cpp -std=c++17 

disassembler.o:	disassemble.cpp
	g++ -c memory.cpp -std=c++17 
	