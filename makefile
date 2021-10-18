disassemble:	disassemble.o memory.o
	g++ disassemble.o memory.o -o disassemble

disassemble.o:	disassemble.cpp
	g++ -c disassemble.cpp -I./ -std=c++17 

memory.o:		memory.cpp
	g++ -c memory.cpp -std=c++17 
	