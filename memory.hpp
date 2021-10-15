/*
 * Template class for Memory objects for use in vintage computer emulators
 */

#include <vector>
#include <cstdint>
#include <memory>

class Memory {
    public:
        // return the word at address
        virtual uint8_t read(uint16_t address) const; 
        // write word to address, obey write protections
        // models writing to RAM
        virtual void write(uint8_t word, uint16_t address); 
        // write word to address, disegard write protections
        // models "flashing" a ROM
        virtual void load(uint8_t word, uint16_t address); 
        // create a memory holding a number of words
        Memory(int words);
        // create a memory and load information
        Memory(std::unique_ptr<std::vector<uint8_t>> data);
        // set offset to first memory cell
        void setStartOffset(uint16_t offset);
        virtual ~Memory();
        // low address of memory
        uint16_t getLowAddress();
        // high address of memory
        uint16_t getHighAddress();
    protected:
        int words; // size of memory buffer in words
        uint16_t startOffset; // offset to beginning of address range
        std::unique_ptr<std::vector<uint8_t>> contents;
};