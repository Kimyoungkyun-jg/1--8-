#pragma once
#include "UObject.h"
#include <vector>

using namespace std;

class TotalManager : public UObject //��ӿ� 
{
public: 
	static TotalManager& GetInstance() //�̱��� �������� ����
	{
		static TotalManager instance;
		return instance;
	}

	TotalManager ( ){}

	TotalManager(const TotalManager&) = delete;
	TotalManager& operator=(const TotalManager) = delete;
};

