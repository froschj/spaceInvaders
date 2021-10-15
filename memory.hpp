/*
 * Template class for Memory objects for use in vintage computer emulators
 */

#include <vector>

template <typename addressType, typename wordType>
class Memory {
    public:
        // return the word at address
        virtual wordType read(addressType address) const; 
        // write word to address, obey write protections
        // models writing to RAM
        virtual void write(wordType word, addressType address); 
        // write word to address, disegard write protections
        // models "flashing" a ROM
        virtual void load(wordType word, addressType address); 
        // create a memory holding a number of words
        Memory(int words);
        // set offset to first memory cell
        void setStartOffset(addressType offset);
    protected:
        int words; // size of memory buffer in words
        addressType startOffset; // offset to beginning of address range
        std::vector<wordType> contents;
};
