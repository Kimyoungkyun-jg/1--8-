#pragma once

#include "UObject.h"
#include <vector>


extern std::vector<UObject> AllObjects;

class TotalManager : public UObject
{
public:
	static TotalManager& GetInstance()
	{
		static TotalManager instance;
		return instance;
	}

	TotalManager(const TotalManager&) = delete;
	TotalManager& operator=(const TotalManager&) = delete;

protected:
	TotalManager() : UObject()
	{
	}
	virtual ~TotalManager() override {}
};
