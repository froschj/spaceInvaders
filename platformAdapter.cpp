#include "platformAdapter.hpp"
//https://www.dummies.com/programming/c/how-to-use-hex-with-binary-for-c-programming/

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


void Adapter::setExplosionSoundFunction(std::function<void()> func)
{
	explosionFunc = func;
}

void Adapter::playSoundExplosion()
{
	if (explosionFunc)
	{
		explosionFunc();
	}
}

void Adapter::setFastInvaderFunction1(std::function<void()> func)
{
	fastInvader1Func = func;
}

void Adapter::playSoundFastInvader1()
{
	if (fastInvader1Func)
	{
		fastInvader1Func();
	}
}

void Adapter::setFastInvaderFunction2(std::function<void()> func)
{
	fastInvader2Func = func;
}

void Adapter::playSoundFastInvader2()
{
	if (fastInvader2Func)
	{
		fastInvader2Func();
	}
}

void Adapter::setFastInvaderFunction3(std::function<void()> func)
{
	fastInvader3Func = func;
}

void Adapter::playSoundFastInvader3()
{
	if (fastInvader3Func)
	{
		fastInvader3Func();
	}
}

void Adapter::setFastInvaderFunction4(std::function<void()> func)
{
	fastInvader4Func = func;
}

void Adapter::playSoundFastInvader4()
{
	if (fastInvader4Func)
	{
		fastInvader4Func();
	}
}

void Adapter::setInvaderKilledFunction(std::function<void()> func)
{
	invaderKilledFunc = func;
}

void Adapter::playSoundInvaderKilled()
{
	if (invaderKilledFunc)
	{
		invaderKilledFunc();
	}
}

void Adapter::setShootFunction(std::function<void()> func)
{
	shootFunc = func;
}

void Adapter::playSoundShoot()
{
	if (shootFunc)
	{
		shootFunc();
	}
}

void Adapter::setUFOHighPitchFunction(std::function<void()> func)
{
	ufoHighPitchFunc = func;
}

//TODO I believe the ufo needs a start/stop, and to loop on the platform side
void Adapter::playSoundUFOHighPitch()
{
	if (ufoHighPitchFunc)
	{
		ufoHighPitchFunc();
	}
}

//TODO I believe the ufo needs a start/stop, and to loop on the platform side
void Adapter::setUFOLowPitchFunction(std::function<void()> func)
{
	ufoLowPitchFunc = func;
}

void Adapter::playSoundUFOLowPitch()
{
	if (ufoLowPitchFunc)
	{
		ufoLowPitchFunc();
	}
}

void Adapter::coin()
{
	port1 |= 0x01; //set bit 0
}

void Adapter::p2StartButton()
{
	port1 |= 0x02; //set bit 1
}

void Adapter::p1StartButton()
{
	port1 |= 0x04; //set bit 2
}

void Adapter::p1ShootButton()
{
	port1 |= 0x08; //set bit 4
}

void Adapter::p1Left()
{
	port1 |= 0x20; //set bit 5
}

void Adapter::p1Right()
{
	port1 |= 0x40; //set bit 6
}

void Adapter::p2ShootButton()
{
	port2 |= 0x08; //set bit 4
}

void Adapter::p2Left()
{
	port2 |= 0x20; //set bit 5
}

void Adapter::p2Right()
{
	port2 |= 0x40; //set bit 6
}

const uint8_t Adapter::getPort1()
{
	return port1;
}

const uint8_t Adapter::getPort2()
{
	return port2;
}

void Adapter::setPort1(uint8_t port1In)
{
	port1 = port1In;
}

void Adapter::setPort2(uint8_t port2In)
{
	port2 = port2In;
}
