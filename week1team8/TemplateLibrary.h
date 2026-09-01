#pragma once

#include <d3d11.h>
#include "Vector.h"
#include "UObject.h"
#include "ObjectManager.h"


template<class T>
inline T* NewObject()
{
	static_assert(std::is_base_of_v<UObject, T>);

	T* Obj = new T;
	UObjectManager::Get().AllObjects.push_back(Obj);

	return static_cast<T*>(Obj);
}

template<class T>
inline T* SpawnActor(FVector Location, EPrimitive Primitive, FVector Scale = { 1, 1, 1 })
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
inline T* SpawnColider(FVector Location, EPrimitive Primitive, FVector Scale = {1, 1, 1})
{
	static_assert(std::is_base_of_v<ACollider, T>);
	ACollider* Colider = SpawnActor<T>(Location, Primitive, Scale);

	// 속력
	float minSpeed = 1.0f;
	float maxSpeed = 5.0f;
	float speed = (static_cast<float>(rand()) / RAND_MAX) * (maxSpeed - minSpeed) + minSpeed;

	// 방향
	float PI = acos(-1.0f);
	float radian = (static_cast<float>(rand()) / RAND_MAX) * 2 * PI;
	FVector direction = FVector(cos(radian), sin(radian));

	// 속도
	Colider->SetVelocity(direction * speed);
	return static_cast<T*>(Colider);
}