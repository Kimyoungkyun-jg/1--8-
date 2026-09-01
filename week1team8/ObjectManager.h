#pragma once

#include <vector>
#include "UObject.h"
#include "CollisionManager.h"

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
				if (ACollider* Collider = dynamic_cast<ACollider*>(Target))
				{
					CollisionManager::GetInstance().DeleteColider(Collider->GetID());
				}

				UObject* temp = AllObjects[i];
				std::swap(AllObjects[i], AllObjects.back());
				AllObjects.pop_back();
				delete(temp);

				break;
			}
		}
	}

	void DestroyAllObjects()
	{
		AllObjects.clear();
	}

	UObject* Find(int ID)
	{
		for (UObject* Obj : AllObjects)
		{
			if (Obj->GetID() == ID)
				return Obj;
		}

		return nullptr;
	}

	static UObjectManager& Get()
	{
		static UObjectManager Manager;
		return Manager;
	}
	UObjectManager(const UObjectManager&) = delete;
	UObjectManager& operator=(const UObjectManager&) = delete;

private:
	UObjectManager(){}
};
