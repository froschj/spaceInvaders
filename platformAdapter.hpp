#pragma once
#include <functional>

/*
Read 1
BIT 0   coin(0 when active)
1   P2 start button
2   P1 start button
3 ?
4   P1 shoot button
5   P1 joystick left
6   P1 joystick right
7 ?

Read 2
BIT 0, 1 dipswitch number of lives(0:3, 1 : 4, 2 : 5, 3 : 6)
2   tilt 'button'
3   dipswitch bonus life at 1 : 1000, 0 : 1500
4   P2 shoot button
5   P2 joystick left
6   P2 joystick right
7   dipswitch coin info 1 : off, 0 : on
*/

class Adapter
{
	private:
		uint8_t port1;
		uint8_t port2;

		std::function<void()> invokeFunc;

		std::function<void()> explosionFunc;
		std::function<void()> fastInvader1Func;
		std::function<void()> fastInvader2Func;
		std::function<void()> fastInvader3Func;
		std::function<void()> fastInvader4Func;
		std::function<void()> invaderKilledFunc;
		std::function<void()> shootFunc;
		std::function<void()> ufoHighPitchFunc;
		std::function<void()> ufoLowPitchFunc;

	public:

		//Set sound callback functions (platform sets)
		void setExplosionSoundFunction(std::function<void()> func);
		void setFastInvaderFunction1(std::function<void()> func);
		void setFastInvaderFunction2(std::function<void()> func);
		void setFastInvaderFunction3(std::function<void()> func);
		void setFastInvaderFunction4(std::function<void()> func);
		void setInvaderKilledFunction(std::function<void()> func);
		void setShootFunction(std::function<void()> func);
		void setUFOHighPitchFunction(std::function<void()> func);
		void setUFOLowPitchFunction(std::function<void()> func);

		//Callback functions for sounds (machine calls)
		void playSoundExplosion();		
		void playSoundFastInvader1();		
		void playSoundFastInvader2();		
		void playSoundFastInvader3();		
		void playSoundFastInvader4();		
		void playSoundInvaderKilled();		
		void playSoundShoot();		
		void playSoundUFOHighPitch();		
		void playSoundUFOLowPitch();

		//Port bit set functions
		void coin();
		void p2StartButton();
		void p1StartButton();
		void p1ShootButton();
		void p1Left();
		void p1Right();
		void p2ShootButton();
		void p2Left();
		void p2Right();
		const uint8_t getPort1();
		const uint8_t getPort2();
		void setPort1(uint8_t port1In);
		void setPort2(uint8_t port2In);
};

