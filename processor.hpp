/*
 * Interface for generic processor class and specializations
 */
#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP


#include <memory>
#include <cstdint>
#include <exception>
#include <string>


/*
 * A meaningful exception to throw if there is an out-of-bounds 
 * memory read by the "processor"
 */
class MemoryReadError : public std::exception {
    private:
        std::string msg;
    public:
        MemoryReadError(const std::string& address) : 
                msg(std::string("Invalid read at address: ") + address){}
        virtual const char *what() const throw() {
            return msg.c_str();
        }
};

/*
 * A meaningful exception to throw if the "processor" encounters an
 * unknown or unimplemented opcode
 */
class UnimplememntedInstructionError : public std::exception {
    private:
        std::string msg;
    public:
        UnimplememntedInstructionError(
                const std::string& address,
                const std::string& opcode 
        ) : 
                msg(
                    std::string("Invalid opcode ") 
                    + opcode + 
                    std::string(" at address: ") 
                    + address){}
        virtual const char *what() const throw() {
            return msg.c_str();
        }
};

/* https://stackoverflow.com/questions/6924754/return-type-covariance-with-smart-pointers */
/*
 * Generic State to derive specific processor states (registers) from
 */
struct State {
    private:
        const char *id = "state";
        virtual struct State* doClone() const {
            struct State *temp = new struct State;
            return temp;
        }
    public:
        virtual ~State(){}
        std::unique_ptr<struct State> clone() const {
            return std::unique_ptr<struct State>(doClone());
        }
};

/*
 * Generic Porcessor class
 */
template<class stateType, class memoryType>
class Processor {
    public:
        // execute an instruction, return # of cycles
        virtual int step() = 0; 
        virtual ~Processor() {}
        Processor() {}
        std::unique_ptr<stateType> getState() const {
            return state.clone();
        } // return a copy of the current state
        void connectMemory(memoryType *memoryDevice) {
            memory = memoryDevice;
        }; // connect the processor to a memory
    protected:
        stateType state;
        memoryType *memory;
};

#endif