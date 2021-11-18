/*
CS467 - Build an emulator and run space invaders rom
Jon Frosch & Phil Sheets

Machine emulator. This class emulates the arcade cabinet for space invaders.
It provides a step() function to advance the CPU emulator and trigger
interrupts for each half and end frame. Input is processed via setting
bits in the emulated ports. Output is also handled when emulator writes
port values, which will trigger sounds in the platform adapter.

See computer archeology for information on the port handling for I/O
and shift register
http://computerarcheology.com/Arcade/SpaceInvaders/Hardware.html
https://en.cppreference.com/w/cpp/chrono/time_point
https://www.dummies.com/programming/c/how-to-use-hex-with-binary-for-c-programming/
*/

#include "machine.hpp"
#include "platformAdapter.hpp"
#include "emulator.hpp"
#include <chrono>



Machine::Machine() : _emulator(0), _platformAdapter(0), _port1(0), _port2(0), 
					shiftRegister(0), shiftRegisterOffset(0), cycleCount(0),
					_prev_port3(0), _prev_port5(0)
{
	_frameStartTime = std::chrono::high_resolution_clock::now();
}

//Sets the cpu emulator used by this machine.
//Also sets the port input/output callback functions for the cpu emulator.
void Machine::setEmulator(Emulator8080* emulator)
{
	_emulator = emulator;
	
	_emulator->connectInput([this](uint8_t port) { return this->readPortValue(port); });
	_emulator->connectOutput([this](uint8_t port, uint8_t value) { this->writePortValue(port, value); });	
}

void Machine::setPlatformAdapter(Adapter* platformAdapter)
{
	_platformAdapter = platformAdapter;
}

//Main processing loop of the machine. Processes input to change
//bits in the port values. Determines if enough time has elapsed to tirgger
//a half-frame (RST2) or end-of-frame (RST1) interrupt. If so, advances
//the cpu emulator the correct number of cycles for the elapsed time.
void Machine::step()
{
	if (!_emulator)
	{
		return;
	}

	//Check platform for input
	if (_platformAdapter->isInputChanged())
	{
		processInput();
	}

	//Check time for refresh
	//https://www.geeksforgeeks.org/measure-execution-time-function-cpp/
	auto checkTime = std::chrono::high_resolution_clock::now();
	auto duration = std::chrono::duration_cast<std::chrono::microseconds>(checkTime - _frameStartTime);
		
	//Framerate is 60hz. Interrupt the cpu twice per frame (mid-frame and at end)
	//Redraw the screen at end of every frame.
	if (duration.count() > 8333) //16666 microseconds in a 1/60 sec frame, check for interrupt every half frame
	{
		bool drawScreen = false;		
		bool isInterruptable = _emulator->isInterruptEnable();

		if (isInterruptable)
		{	
			if (useRST1)
			{	//End of frame interrupt
				_emulator->requestInterrupt(RST1); 
				drawScreen = true;
			}
			else
			{
				//mid-frame interrupt
				_emulator->requestInterrupt(RST2);
			}
			
			useRST1 = !useRST1;
		}
		
		//Catch up the CPU for the time that has passed
		//This emulator should be a 2mhz speed. Since we advance twice a frame,
		//this means that 1mhz should pass - which is the same as the microseconds
		//value that has elapsed. So advance the cpu 1 cycle for every microsecond.
		cycleCount = 0;
		uint64_t cycles = duration.count();
		while (cycleCount < cycles)
		{
			//Step emulator
			cycleCount += _emulator->step();
		}	
		
		if (drawScreen)
		{
			_platformAdapter->refreshScreen();		
		}

		_frameStartTime = std::chrono::high_resolution_clock::now();
	}

}

//Sets the emulated port bits for the input based on the platform
//adapter settings.
void Machine::processInput()
{
	setCoinBit(_platformAdapter->isCoin());
	
	//Start bits
	setP1StartButtonBit(_platformAdapter->isP1StartButtonDown());
	setP2StartButtonBit(_platformAdapter->isP2StartButtonDown());	
	
	//P1 bits
	setP1ShootButtonBit(_platformAdapter->isP1ShootButtonDown());
	setP1LeftBit(_platformAdapter->isP1LeftButtonDown());
	setP1RightBit(_platformAdapter->isP1RightButtonDown());
	
	//P2 bits
	setP2ShootButtonBit(_platformAdapter->isP2ShootButtonDown());
	setP2LeftBit(_platformAdapter->isP2LeftButtonDown());
	setP2RightBit(_platformAdapter->isP2RightButtonDown());
}

void Machine::setCoinBit(bool isSet)
{
	if (isSet)
	{
		_port1 |= 0x01; //set bit 0
	}
	else
	{
		_port1 &= 0xFE; //unset bit 0
	}
}

void Machine::setP2StartButtonBit(bool isSet)
{
	if (isSet)
	{
		_port1 |= 0x02; //set bit 1
	}
	else
	{
		_port1 &= 0xFD; //unset bit 1
	}

}

void Machine::setP1StartButtonBit(bool isSet)
{
	if (isSet)
	{
		_port1 |= 0x04; //set bit 2	
	}
	else
	{
		_port1 &= 0xFB; //unset bit 2
	}
	
	
}

void Machine::setP1ShootButtonBit(bool isSet)
{
	if (isSet)
	{
		_port1 |= 0x10; //set bit 4
	}
	else
	{
		_port1 &= 0xEF; //unset bit 4
	}

	
}

void Machine::setP1LeftBit(bool isSet)
{
	if (isSet)
	{
		_port1 |= 0x20; //set bit 5
	}
	else
	{
		_port1 &= 0xDF; //unset bit 5
	}

	
}

void Machine::setP1RightBit(bool isSet)
{
	if (isSet)
	{
		_port1 |= 0x40; //set bit 6
	}
	else
	{
		_port1 &= 0xBF; //unset bit 6
	}

	
}

void Machine::setP2ShootButtonBit(bool isSet)
{
	if (isSet)
	{
		_port2 |= 0x10; //set bit 4
	}
	else
	{
		_port2 &= 0xEF; //unset bit 4
	}

	
}

void Machine::setP2LeftBit(bool isSet)
{
	if (isSet)
	{
		_port2 |= 0x20; //set bit 5
	}
	else
	{
		_port2 &= 0xDF; //unset bit 5
	}

	
}

void Machine::setP2RightBit(bool isSet)
{
	if (isSet)
	{
		_port2 |= 0x40; //set bit 6
	}
	else
	{
		_port2 &= 0xBF; //unset bit 6
	}
	
}

//Emulates writing port values to the machine. The cpu emulator
//writes to the port when:
// port 2: to write the offset value used in the shift register
// port 3 & 5: to trigger sounds in the machine
// port 4: to write a value to the shift register
// parameter port: Which port to write
// parameter value: new value for the port
void Machine::writePortValue(uint8_t port, uint8_t value)
{
	switch (port)
	{
		case 2: //shift value
			/*
			;    Writing to port 2 (bits 0, 1, 2) sets the offset for the 8 bit result, eg.
			*/
			shiftRegisterOffset = (value & 0x7); //Use only right 3 bits
			break;
		case 3: //sounds
				/*
				bit 0 = UFO(repeats)        SX0 0.raw
				bit 1 = Shot                 SX1 1.raw
				bit 2 = Flash(player die)   SX2 2.raw
				bit 3 = Invader die          SX3 3.raw
				*/

			//Only play sounds when first changed
			if (value != _prev_port3)
			{
				//the UFO sound repeats
				if (value & 0x01 && !(_prev_port3 & 0x01))
				{
					//bit 0 - ufo sound started
					_platformAdapter->startSoundUFO();
				}
				if (!(value & 0x01) && (_prev_port3 & 0x01))
				{
					//bit 0 - ufo sound stopped
					_platformAdapter->stopSoundUFO();
				}
				 if (value & 0x02 && !(_prev_port3 & 0x02))
				{
					//bit 1
					_platformAdapter->playSoundShoot();
				}
				 if (value & 0x04 && !(_prev_port3 & 0x04))
				{
					_platformAdapter->playSoundPlayerDie();
				}
				 if ((value & 0x08) && !(_prev_port3 & 0x08))
				{
					_platformAdapter->playSoundInvaderDie();
				}

				_prev_port3 = value;
			}
			break;
		case 4:
			/*
				; 16 bit shift register:
				;    f              0    bit
				;    xxxxxxxxyyyyyyyy
				;
				;    Writing to port 4 shifts x into y, and the new value into x, eg.
				;    $0000,
				;    write $aa->$aa00,
				;    write $ff->$ffaa,
				;    write $12->$12ff, ..
			*/
			uint8_t rightHalf;
			rightHalf = (shiftRegister >> 8);
			shiftRegister = (value << 8);
			shiftRegister |= rightHalf;
			break;
		case 5:
			/*
			bit 0 = Fleet movement 1     SX6 4.raw
			bit 1 = Fleet movement 2     SX7 5.raw
			bit 2 = Fleet movement 3     SX8 6.raw
			bit 3 = Fleet movement 4     SX9 7.raw
			bit 4 = UFO Hit              SX10 8.raw
			*/
			//Play sounds, only when changes occur
			if (value != _prev_port5)
			{
				if (value & 0x01 && !(_prev_port5 & 0x01))
				{
					_platformAdapter->playSoundFleetMove1();
				}
				if (value & 0x02 && !(_prev_port5 & 0x02))
				{
					_platformAdapter->playSoundFleetMove2();
				}
				if (value & 0x04 && !(_prev_port5 & 0x04))
				{
					_platformAdapter->playSoundFleetMove3();
				}
				if (value & 0x08 && !(_prev_port5 & 0x08))
				{
					_platformAdapter->playSoundFleetMove4();
				}
				if (value & 0x10 && !(_prev_port5 & 0x10))
				{
					_platformAdapter->playSoundUFOHit();
				}

				_prev_port5 = value;
			}
			break;
		default:
			break;
			
	}
}

//Emulates reading port values from the machine.
//The cpu emulator will read values for input, or for the result
//of the "hardware" shift register
// port 1&2: input ports
// port 3: result of shift register
// parameter port: Which port to read.
uint8_t Machine::readPortValue(uint8_t port)
{
	uint8_t value{0};
	switch (port)
	{
		case 1:
			value = _port1;
			break;
		case 2:
			value = _port2;
			break;
		case 3:
			//Reading from port 3 returns the shift register shifted by the offset
			/*
			;    Writing to port 2 (bits 0, 1, 2) sets the offset for the 8 bit result, eg.
			;    offset 0:
			;    rrrrrrrr        result = xxxxxxxx
			;    xxxxxxxxyyyyyyyy
			;
			;    offset 2:
			;      rrrrrrrr  result = xxxxxxyy
			;    xxxxxxxxyyyyyyyy
			;
			;    offset 7:
			;           rrrrrrrr result = xyyyyyyy
			;    xxxxxxxxyyyyyyyy
			*/
			//Start with moving the bits "right" over 8 to return the upper 8 bits
			int offset;
			offset = 8;
			//Apply the shift offset in the opposite direction
			offset -= shiftRegisterOffset;
			//shift and return 8bit value
			value = (shiftRegister >> offset) & 0xFF;

			break;
		default:
			break;
	}

	return value;
}
