#pragma once

#include "UObject.h"

class EffectManager
{
	static EffectManager& GetInstance()
	{
		static EffectManager instance;
		return instance;
	}
};

