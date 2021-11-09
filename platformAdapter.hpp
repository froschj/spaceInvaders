#pragma once
#include <functional>


class Adapter
{
	private:
		//Sound functions
		std::function<void()> playerDieFunc;
		std::function<void()> fleetMove1Func;
		std::function<void()> fleetMove2Func;
		std::function<void()> fleetMove3Func;
		std::function<void()> fleetMove4Func;
		std::function<void()> invaderDieFunc;
		std::function<void()> shootFunc;
		std::function<void()> startUFOFunc;
		std::function<void()> stopUFOFunc;
		std::function<void()> ufoHitFunc;

		//Visual functions
		std::function<void()> refreshScreenFunc;

		bool _inputChanged;

		bool _coin;
		bool _p2StartButtonDown;
		bool _p1StartButtonDown;
		bool _p1ShootButtonDown;
		bool _p1LeftDown;
		bool _p1RightDown;
		bool _p2ShootButtonDown;
		bool _p2LeftDown;
		bool _p2RightDown;

	public:

		//** Input functions
		bool isInputChanged();
		void setInputChanged(bool inputChanged);
		void setCoin(bool coin);
		bool isCoin();
		void setP2StartButtonDown(bool down);
		bool isP2StartButtonDown();
		void setP1StartButtonDown(bool down);
		bool isP1StartButtonDown();
		void setP1ShootButtonDown(bool down);
		bool isP1ShootButtonDown();
		void setP1LeftButtonDown(bool down);
		bool isP1LeftButtonDown();
		void setP1RightButtonDown(bool down);
		bool isP1RightButtonDown();
		void setP2ShootButtonDown(bool down);
		bool isP2ShootButtonDown();
		void setP2LeftButtonDown(bool down);
		bool isP2LeftButtonDown();
		void setP2RightButtonDown(bool down);
		bool isP2RightButtonDown();
		
		//** Callback functions

		void setRefreshScreenFunction(std::function<void()> func);
		void refreshScreen();

		//Set sound callback functions (platform sets)
		void setPlayerDieSoundFunction(std::function<void()> func);
		void setFleetMove1Function(std::function<void()> func);
		void setFleetMove2Function(std::function<void()> func);
		void setFleetMove3Function(std::function<void()> func);
		void setFleetMove4Function(std::function<void()> func);
		void setInvaderDieFunction(std::function<void()> func);
		void setShootFunction(std::function<void()> func);
		void setStartUFOFunction(std::function<void()> func);
		void setStopUFOFunction(std::function<void()> func);
		void setUFOHitFunction(std::function<void()> func);

		//Callback functions for sounds (machine calls)
		void playSoundPlayerDie();		
		void playSoundFleetMove1();		
		void playSoundFleetMove2();		
		void playSoundFleetMove3();		
		void playSoundFleetMove4();		
		void playSoundInvaderDie();		
		void playSoundShoot();		
		//void playSoundUFO(bool start);		
		void startSoundUFO();
		void stopSoundUFO();
		void playSoundUFOHit();

};

