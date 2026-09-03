#pragma once

#include <d3d11.h>
#include "Vector.h"
#include "UObject.h"
#include "ObjectManager.h"
#include "CollisionManager.h"
#include <vector>
#include <cmath>


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

// Point에서 물체 표면까지의 거리. 물체 안에 있으면 0.
// 중심까지의 거리로 재면 긴 판자가 폭심에 몸통을 걸치고도 중심이 밖이라 빠진다
inline float DistanceToSurface(const ACollider* Collider, FVector Point)
{
	if (Collider->GetPrimitive() == EPrimitive::Circle)
	{
		// 원은 중심 거리에서 반지름만 빼면 된다 (ACircle::GetRadius와 같은 식)
		float Distance = (Collider->GetLocation() - Point).Length() - Collider->GetScale().x * 0.5f;
		return std::fmax(Distance, 0.0f);
	}

	// 사각형은 Point를 로컬 축에 투영한 뒤 변 안쪽으로 잘라내면 가장 가까운 점이 나온다.
	// 회전한 사각형도 축이 같이 돌아가므로 그대로 성립한다
	OBB Box = MakeOBB(Collider);
	FVector Offset = Point - Box.center;
	FVector Closest = Box.center;

	for (int i = 0; i < 2; i++)
	{
		float t = Offset.DotProduct(Box.axis[i]);
		t = std::fmax(-Box.half[i], std::fmin(t, Box.half[i]));
		Closest = Closest + Box.axis[i] * t;
	}

	return (Point - Closest).Length();
}

inline bool TraceSphere(FVector Location, float Radius, std::vector<ACollider*>& Result)
{
	bool bFound = false;
	std::vector<ACollider*> Colliders = CollisionManager::GetInstance().colliders;
	for (ACollider* c : Colliders)
	{
		// 중심이 원 안에 있는지가 아니라, 물체가 원과 겹치는지를 본다
		if (DistanceToSurface(c, Location) <= Radius)
		{
			Result.push_back(c);
			bFound = true;
		}
	}

	return bFound;
}