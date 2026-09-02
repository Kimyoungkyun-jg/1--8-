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
	FVector rA, rB;              // 질량중심 -> 접촉점 (지금 매번 계산하던 것)
	FVector tangent;             // 접선 방향 (고정)
	float normalMass = 0.0f;     // 1 / validMass       (미리 나눠둔 값)
	float tangentMass = 0.0f;    // 1 / validMassTangent
	float velocityBias = 0.0f;   // 목표 분리 속도

	float normalImpulse = 0.0f;  // 누적 충격량 1: 법선
	float tangentImpulse = 0.0f; // 누적 충격량 2: 마찰
	float initialNormalVelocity = 0.0f;
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

	float InvInertia(float mass);

	std::vector<ACollider*> colliders;
	std::vector<ACollider*> pendingkills;
	float CollisionThreshold = 100.0f;

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

	void SetAllCollisionFriction(float _dynamic, float _static);

	std::vector<CollisionInfo> CheckCollisionAll();

	// 충돌 감지
	CollisionInfo CheckCollision(ACollider* a, ACollider* b);

	// 위치, 크기(가로, 세로, 반지름), 회전, 속도, 무게 필요
	CollisionInfo CheckCollisionCircleCircle(ACollider* a, ACollider* b);

	CollisionInfo CheckCollisionRectangleRectangle(ACollider* a, ACollider* b);

	// a == Circle, b == Rectangle
	CollisionInfo CheckCollisionCircleRectangle(ACollider* a, ACollider* b);

	void ResolvePosition(ACollider* a, ACollider* b, const CollisionInfo& info);

	// 충돌해결
	void SolveContact(ACollider* a, ACollider* b, CollisionInfo& info);
	void InitContact(ACollider* a, ACollider* b, CollisionInfo& info);
};
