#pragma once

#include <vector>
#include "UObject.h"

//모든 UObject를 관리하는 클래스, Main 초기에 Get 호출
class UObjectManager
{
public:

	~UObjectManager()
	{
		DestroyAllObjects();
	}

	std::vector<UObject*> AllObjects;
	void Destroy(UObject* Target)
	{
		for (int i = AllObjects.size() - 1;i >= 0; --i)
		{
			if (AllObjects[i] == Target)
			{
				UObject* temp = AllObjects[i];
				std::swap(AllObjects[i], AllObjects.back());
				AllObjects.pop_back();
				delete(temp);
			}
		}
	}

	void DestroyAllObjects()
	{
		AllObjects.clear();
	}

	static UObjectManager& Get()
	{
		static UObjectManager Manager;
		return Manager;
	}

private:
	UObjectManager(){}
};
