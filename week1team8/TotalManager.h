#pragma once

#include "UObject.h"
#include <vector>

using namespace std;

class TotalManager : public UObject // 상속용 
{
public:
	static TotalManager& GetInstance() // 싱글톤 패턴으로 관리
	{
		static TotalManager instance;
		return instance;
	}

	TotalManager(const TotalManager&) = delete;
	TotalManager& operator=(const TotalManager) = delete;

private:
	TotalManager()
	{
	}
};

