#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

#include "UObject.h"


struct CollisionInfo
{
	FVector contactPoint = FVector();
	FVector normal = FVector();
	float penetration = 0.0f;
	bool isCollision = false;
	EColliderId colAId;
	EColliderId colBId;
};

class CollisionManager
{
public:

	static CollisionManager& GetInstance();

	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;
	CollisionManager();
	~CollisionManager();

	float InvMass(float mass);

	std::vector<ACollider*> colliders;
	std::vector<ACollider*> pendingkills;

	void AddColider(ACollider* col)
	{
		colliders.push_back(col);
	}

	bool DeleteColider(int id)
	{
		for (int i = 0; i < colliders.size(); i++)
		{
			if (colliders[i]->GetID() == id)
			{
				std::swap(colliders[i], colliders.back());
				colliders.pop_back();
				return true;
			}
		}

		return false;
	}


	std::vector<CollisionInfo> CheckCollisionAll();

	// 충돌 감지
	CollisionInfo CheckCollision(ACollider* a, ACollider* b);

	// 위치, 크기(가로, 세로, 반지름), 회전, 속도, 무게 필요
	CollisionInfo CheckCollisionCircleCircle(ACollider* a, ACollider* b);

	CollisionInfo CheckCollisionRectangleRectangle(ACollider* a, ACollider* b);

	// a == Circle, b == Rectangle
	CollisionInfo CheckCollisionCircleRectangle(ACollider* a, ACollider* b);

	// 충돌해결
	float ResolveCollision(ACollider* a, ACollider* b, const CollisionInfo& info);
};
