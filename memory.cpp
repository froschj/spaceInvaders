#ifndef MEMORY_HPP
#define MEMORY_HPP
#include "memory.hpp"

template <typename addressType, typename wordType>
Memory<addressType, wordType>::Memory(int words) : contents(words, 0) {
    this->words = words;
    this->startOffset = 0;
}

template <typename addressType, typename wordType>
wordType Memory<addressType, wordType>::read(addressType address) const {
    return contents.at(address);
}

template <typename addressType, typename wordType>
void Memory<addressType, wordType>::write(wordType word, addressType address) {
    this->load(word, address);
}

template <typename addressType, typename wordType>
void Memory<addressType, wordType>::load(wordType word, addressType address) {
    contents.at(address) = word;
}

template <typename addressType, typename wordType>
void Memory<addressType, wordType>::setStartOffset(addressType offset) {
    startOffset = offset;
}

#endif