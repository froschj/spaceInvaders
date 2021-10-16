/*
 * Interface for generic processor class and specializations
 */
#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP


#include <memory>
#include <cstdint>

/* https://stackoverflow.com/questions/6924754/return-type-covariance-with-smart-pointers */
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

template<class stateType, class memoryType>
class Processor {
    public:
        virtual void step() = 0;
        virtual ~Processor();
        Processor();
        std::unique_ptr<stateType> getState() const {
            return state.clone();
        }
        void connectMemory(memoryType *memoryDevice) {
            memory = memoryDevice;
        };
    protected:
        stateType state;
        memoryType *memory;
};

#endif