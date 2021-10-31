#include <cstdint>
#include <chrono>

class Machine
{

	private:
		class Adapter* _platformAdapter;
		class Emulator8080* _emulator;
		uint8_t _port1;
		uint8_t _port2;
		uint8_t _prev_port3;
		uint8_t _prev_port5;
		std::chrono::time_point<std::chrono::high_resolution_clock> _frameStartTime;

		void processInput();
		
		//Shift register variables
		uint8_t shiftRegisterOffset;
		uint16_t shiftRegister;

		//Timing cycle variables
		int cycleCount;
		const int maxCycles = 1000000;


	public:
		Machine();
		void setEmulator(class Emulator8080 *emulator);
		void setPlatformAdapter(class Adapter *platformAdapter);
		void step();

		//Port bit set functions
		void setCoinBit(bool isSet);
		void setP2StartButtonBit(bool isSet);
		void setP1StartButtonBit(bool isSet);
		void setP1ShootButtonBit(bool isSet);
		void setP1LeftBit(bool isSet);
		void setP1RightBit(bool isSet);
		void setP2ShootButtonBit(bool isSet);
		void setP2LeftBit(bool isSet);
		void setP2RightBit(bool isSet);

		//OUT from emulator into port. Arguments are port, value
		void writePortValue(uint8_t port, uint8_t value);

		//IN from machine into port. Argument is port, machine returns the value
		uint8_t readPortValue(uint8_t port);
};