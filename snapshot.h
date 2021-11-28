#pragma once
#include <cstdint>
#include <vector>
#include <memory>
#include "emulator.hpp"

class Snapshot
{
public:
	std::vector<uint8_t> memoryData;
	std::unique_ptr<State8080> state;

	void copyMemory(Memory* memory)
	{
		memoryData.clear();
		memoryData.reserve(0x4000);
		for (uint16_t i = 0; i < 0x4000; ++i)
		{
			memoryData.push_back(memory->read(i));
		}
	}

	std::unique_ptr<Snapshot> clone() const {
		std::unique_ptr<Snapshot> snapshot = std::make_unique<Snapshot>();
		snapshot->state = this->state->clone();
		snapshot->memoryData = this->memoryData; //vector assignment runs deep copy
		return snapshot;		
	}	
};
