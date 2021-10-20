#pragma once
#include <functional>

class Adapter
{
	private:
		std::function<void()> invokeFunc;
	public:
		void setInvoke(std::function<void()> func);
		void invoke();
};