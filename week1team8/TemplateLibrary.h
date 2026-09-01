#pragma once

#include <d3d11.h>
#include "Vector.h"
#include "UObject.h"
#include "ObjectManager.h"
#include "CollisionManager.h"


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
inline T* SpawnColider(FVector Location, EPrimitive Primitive, bool bUseGravity = true, FVector Scale = { 0.1, 0.1, 1 }, float Mass = 10)
{
	static_assert(std::is_base_of_v<ACollider, T>);
	ACollider* Colider = SpawnActor<T>(Location, Primitive, Scale);

	Colider->bUseGravity = bUseGravity;
	Colider->SetMass(Mass);
	CollisionManager::GetInstance().AddColider(Colider);

	return static_cast<T*>(Colider);
}