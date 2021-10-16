/*
 * Template class for Memory objects for use in vintage computer emulators
 */
#include "memory.hpp"
#include <cstdint>
#include <memory>


Memory::Memory(int words) {
    this->words = words;
    this->startOffset = 0;
    contents = std::make_unique<std::vector<uint8_t>> (words, 0);
}

Memory::Memory(std::unique_ptr<std::vector<uint8_t>> data) {
    this->words = data->size();
    this->startOffset = 0;
    this->contents = std::move(data);
}

Memory::~Memory() {
    
}

uint8_t Memory::read(uint16_t address) const {
    return contents->at(address);
}

void Memory::write(uint8_t word, uint16_t address) {
    this->load(word, address);
}

void Memory::load(uint8_t word, uint16_t address) {
    contents->at(address) = word;
}

void Memory::setStartOffset(uint16_t offset) {
    startOffset = offset;
}

uint16_t Memory::getLowAddress() {
    return startOffset;
}

uint16_t Memory::getHighAddress() {
    return (contents->size() - 1) + startOffset;
}