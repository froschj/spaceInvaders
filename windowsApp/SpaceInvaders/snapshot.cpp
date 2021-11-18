#include "snapshot.h"
#include "memory.hpp"

Snapshot::Snapshot()
{

}

void Snapshot::copyMemory(Memory* memory)
{
	romData.clear();
	romData.reserve(0x4000);
	for (uint16_t i = 0; i < 0x4000; ++i)
	{
		romData.push_back(memory->read(i));
	}
}
