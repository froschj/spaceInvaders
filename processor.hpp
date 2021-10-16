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

struct DisassemblerState8080 : State {
    private:
        virtual struct DisassemblerState8080* doClone() const {
            struct DisassemblerState8080 *temp = 
                new struct DisassemblerState8080;
                temp->pc = this->pc;
                return temp;
        } 
    public:
        uint16_t pc;
        std::unique_ptr<struct DisassemblerState8080> clone() const {
            return std::unique_ptr<struct DisassemblerState8080>(doClone());
        }
};

#endif