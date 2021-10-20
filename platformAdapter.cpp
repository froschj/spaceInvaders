#include "platformAdapter.hpp"

void Adapter::setInvoke(std::function<void()> func)
{
	invokeFunc = func;
}

void Adapter::invoke()
{
	invokeFunc();
}