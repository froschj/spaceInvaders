#include "machine.hpp"
#include "platformAdapter.hpp"

void Machine::setPlatformAdapter(Adapter* platformAdapter)
{
	_platformAdapter = platformAdapter;
}

//TODO test func just to test callback loop
void Machine::playSound()
{
	if (_platformAdapter)
	{
		_platformAdapter->playSoundShoot();
	}
}