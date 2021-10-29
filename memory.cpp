/*
 * Template class for Memory objects for use in vintage computer emulators
 */
#include "memory.hpp"
#include <cstdint>
//#include <memory>

//Empty constructor
Memory::Memory() : words(0), startOffset(0)
{
}

// create a new memory object that can hold a certain number of 8-bit words
Memory::Memory(int words) {
    this->words = words;
    this->startOffset = 0;
    contents = std::make_unique<std::vector<uint8_t>> (words, 0);
}

// acquire a vector of bytes and construct a new memory objetc to hold it
Memory::Memory(std::unique_ptr<std::vector<uint8_t>> data) {
    this->words = data->size();
    this->startOffset = 0;
    this->contents = std::move(data);
}

// empty destructor
Memory::~Memory() {
    
}

// read memory contents at an address
uint8_t Memory::read(uint16_t address) const {
    return contents->at(address);
}

// write a word at a memory address (obeys ROM)
void Memory::write(uint8_t word, uint16_t address) {
    this->load(word, address);
}

// write a word a a memory address (disredards ROM)
void Memory::load(uint8_t word, uint16_t address) {
    contents->at(address) = word;
}

// specifies an offset to the start of "memory"
void Memory::setStartOffset(uint16_t offset) {
    startOffset = offset;
}

// returns the low address of memory
uint16_t Memory::getLowAddress() {
    return startOffset;
}

// returns the high address of memory
uint16_t Memory::getHighAddress() {
    return (contents->size() - 1) + startOffset;
}

void Memory::setMemoryBlock(std::unique_ptr<std::vector<uint8_t>> data)
{
	this->contents = std::move(data);
}
