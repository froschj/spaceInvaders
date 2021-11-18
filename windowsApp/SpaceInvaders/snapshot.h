#pragma once
#include <cstdint>
#include <vector>
#include "emulator.hpp"
class Snapshot
{
public:
	Snapshot();
	uint8_t flagsRegister;
	uint8_t     a;
	uint8_t     b;
	uint8_t     c;
	uint8_t     d;
	uint8_t     e;
	uint8_t     h;
	uint8_t     l;
	uint16_t    sp;
	uint16_t    pc;
	std::vector<uint8_t> romData;
	void copyMemory(class Memory* memory);
	State8080 state;
};