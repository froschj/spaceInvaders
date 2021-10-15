/*
 * Template class for Memory objects for use in vintage computer emulators
 */

#ifndef MEMORY_HPP
#define MEMORY_HPP
#include "memory.hpp"
#include <cstdint>


Memory::Memory(int words) : contents(words, 0) {
    this->words = words;
    this->startOffset = 0;
}

Memory::~Memory() {
    
}

uint8_t Memory::read(uint16_t address) const {
    return contents.at(address);
}

void Memory::write(uint8_t word, uint16_t address) {
    this->load(word, address);
}

void Memory::load(uint8_t word, uint16_t address) {
    contents.at(address) = word;
}

void Memory::setStartOffset(uint16_t offset) {
    startOffset = offset;
}

uint16_t Memory::getLowAddress() {
    return startOffset;
}

uint16_t Memory::getHighAddress() {
    return (contents.size() - 1) + startOffset;
}

#endif