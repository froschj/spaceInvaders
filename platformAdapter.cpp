#include "platformAdapter.hpp"

bool Adapter::isInputChanged()
{
	return _inputChanged;
}

void Adapter::setInputChanged(bool inputChanged)
{
	_inputChanged = inputChanged;
}

void Adapter::setCoin(bool coin)
{
	_coin = coin;
	setInputChanged(true);
}

bool Adapter::isCoin()
{
	return _coin;
}

void Adapter::setP2StartButtonDown(bool down)
{
	_p2StartButtonDown = down;
	setInputChanged(true);
}

bool Adapter::isP2StartButtonDown()
{
	return _p2StartButtonDown;
}

void Adapter::setP1StartButtonDown(bool down)
{
	_p1StartButtonDown = down;
	setInputChanged(true);
}

bool Adapter::isP1StartButtonDown()
{
	return _p1StartButtonDown;
}

void Adapter::setP1ShootButtonDown(bool down)
{
	_p1ShootButtonDown = down;
	setInputChanged(true);
}

bool Adapter::isP1ShootButtonDown()
{
	return _p1ShootButtonDown;
}

void Adapter::setP1LeftButtonDown(bool down)
{
	_p1LeftDown = down;
	setInputChanged(true);
}

bool Adapter::isP1LeftButtonDown()
{
	return _p1LeftDown;
}

void Adapter::setP1RightButtonDown(bool down)
{
	_p1RightDown = down;
	setInputChanged(true);
}

bool Adapter::isP1RightButtonDown()
{
	return _p1RightDown;
}

void Adapter::setP2ShootButtonDown(bool down)
{
	_p2ShootButtonDown = down;
	setInputChanged(true);
}

bool Adapter::isP2ShootButtonDown()
{
	return _p2ShootButtonDown;
}

void Adapter::setP2LeftButtonDown(bool down)
{
	_p2LeftDown = down;
	setInputChanged(true);
}

bool Adapter::isP2LeftButtonDown()
{
	return _p2LeftDown;
}

void Adapter::setP2RightButtonDown(bool down)
{
	_p2RightDown = down;
	setInputChanged(true);
}

bool Adapter::isP2RightButtonDown()
{
	return _p2RightDown;
}

void Adapter::setRefreshScreenFunction(std::function<void()> func)
{
	refreshScreenFunc = func;
}

void Adapter::refreshScreen()
{
	if (refreshScreenFunc)
	{
		refreshScreenFunc();
	}
}

void Adapter::setPlayerDieSoundFunction(std::function<void()> func)
{
	playerDieFunc = func;
}

void Adapter::playSoundPlayerDie()
{
	if (playerDieFunc)
	{
		playerDieFunc();
	}
}

void Adapter::setFleetMove1Function(std::function<void()> func)
{
	fleetMove1Func = func;
}

void Adapter::playSoundFleetMove1()
{
	if (fleetMove1Func)
	{
		fleetMove1Func();
	}
}

void Adapter::setFleetMove2Function(std::function<void()> func)
{
	fleetMove2Func = func;
}

void Adapter::playSoundFleetMove2()
{
	if (fleetMove2Func)
	{
		fleetMove2Func();
	}
}

void Adapter::setFleetMove3Function(std::function<void()> func)
{
	fleetMove3Func = func;
}

void Adapter::playSoundFleetMove3()
{
	if (fleetMove3Func)
	{
		fleetMove3Func();
	}
}

void Adapter::setFleetMove4Function(std::function<void()> func)
{
	fleetMove4Func = func;
}

void Adapter::playSoundFleetMove4()
{
	if (fleetMove4Func)
	{
		fleetMove4Func();
	}
}

void Adapter::setInvaderDieFunction(std::function<void()> func)
{
	invaderDieFunc = func;
}

void Adapter::playSoundInvaderDie()
{
	if (invaderDieFunc)
	{
		invaderDieFunc();
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

void Adapter::setUFOFunction(std::function<void()> func)
{
	ufoFunc = func;
}

void Adapter::playSoundUFO()
{
	if (ufoFunc)
	{
		ufoFunc();
	}
}

void Adapter::setUFOHitFunction(std::function<void()> func)
{
	ufoHitFunc = func;
}

void Adapter::playSoundUFOHit()
{
	if (ufoHitFunc)
	{
		ufoHitFunc();
	}
}

