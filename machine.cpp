#include "machine.hpp"
#include "platformAdapter.hpp"

void Machine::setPlatformAdapter(Adapter* platformAdapter)
{
	_platformAdapter = platformAdapter;
}

void Machine::playSound()
{
	if (_platformAdapter)
	{
		_platformAdapter->invoke();
	}
}