#pragma once

#include <d3d11.h>
#include "Vector.h"
#include "UObject.h"
#include "ObjectManager.h"
#include "CollisionManager.h"
#include <vector>


template<class T>
inline T* NewObject()
{
	static_assert(std::is_base_of_v<UObject, T>);

	T* Obj = new T;
	UObjectManager::GetInstance().AllObjects.push_back(Obj);

	return static_cast<T*>(Obj);
}

template<class T>
inline T* SpawnActor(FVector Location, EPrimitive Primitive, FVector Scale = { 0.1, 0.1, 1 })
{
	static_assert(std::is_base_of_v<AActor, T>);

	AActor* SpawnedActor = NewObject<T>();

	// 크기
	float minRadius = 0.05f;
	float maxRadius = 0.10f;
	SpawnedActor->SetScale(Scale);

	// 위치
	SpawnedActor->SetLocation(Location);

	// 모양
	SpawnedActor->SetPrimitive(Primitive);
	return static_cast<T*>(SpawnedActor);
}

template<class T>
inline T* SpawnColider(FVector Location, EPrimitive Primitive, bool bUseGravity = true, FVector Scale = { 0.1, 0.1, 1 }, float Mass = 10, float hp = 1)
{
	static_assert(std::is_base_of_v<ACollider, T>);
	ACollider* Colider = SpawnActor<T>(Location, Primitive, Scale);

	Colider->bUseGravity = bUseGravity;
	Colider->SetMass(Mass);
	Colider->SetHp(hp);
	CollisionManager::GetInstance().AddColider(Colider);

	return static_cast<T*>(Colider);
}


inline bool TraceSphere(FVector Location, float Radius, std::vector<ACollider*> &Result)
{
	bool bFound = false;
	std::vector<ACollider*> Colliders = CollisionManager::GetInstance().colliders;
	for (ACollider* c : Colliders)
	{
		if ((c->GetLocation() - Location).LengthSquared() <= Radius * Radius)
		{
			Result.push_back(c);
			bFound = true;
		}
	}

	return bFound;
}

inline bool IsValid(UObject* Object)
{
	if (Object && !Object->IsPendingKill())
		return true;

	return false;
}