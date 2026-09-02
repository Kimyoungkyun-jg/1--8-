#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

#include "UObject.h"


// 접촉점 하나. 솔버가 푸는 단위다.
struct ContactPoint
{
	FVector position = FVector();
	float penetration = 0.0f;

	FVector rA, rB;              // 질량중심 -> 접촉점
	float normalMass = 0.0f;     // 1 / validMass       (미리 나눠둔 값)
	float tangentMass = 0.0f;    // 1 / validMassTangent
	float velocityBias = 0.0f;   // 목표 분리 속도

	float normalImpulse = 0.0f;  // 누적 충격량 1: 법선
	float tangentImpulse = 0.0f; // 누적 충격량 2: 마찰
	float initialNormalVelocity = 0.0f;
};

// 한 쌍의 충돌. 법선은 두 물체가 공유하고, 접촉점만 여러 개가 될 수 있다.
// 지금은 항상 1개이고, 5단계에서 면끼리 닿을 때 2개가 된다.
struct CollisionInfo
{
	FVector normal = FVector();  // B -> A 방향
	FVector tangent = FVector(); // 접선 방향 (고정)
	bool isCollision = false;
	EColliderId colAId = EColliderId::NONE;
	EColliderId colBId = EColliderId::NONE;

	int pointCount = 0;
	ContactPoint points[2];

	FVector AverageContactPoint() const
	{
		if (pointCount == 0)
		{
			return FVector();
		}

		FVector sum;
		for (int i = 0; i < pointCount; i++)
		{
			sum += points[i].position;
		}

		return sum / (float)pointCount;
	}
};

// 회전을 포함한 사각형 (Oriented Bounding Box).
// 지금은 그리기용으로만 쓰고, 다음 단계에서 SAT 판정의 입력이 된다.
struct OBB
{
	FVector center;
	FVector axis[2];    // 회전한 로컬 x축, y축 (단위벡터)
	float half[2];      // 반너비, 반높이
	FVector vertex[4];  // 반시계 방향: 좌하, 우하, 우상, 좌상
};

OBB MakeOBB(const ACollider* collider);

// SAT 판정 결과
struct SATResult
{
	bool overlapped = false;
	FVector normal;              // B -> A 방향 (a를 밀어낼 방향)
	float penetration = 0.0f;    // 그 방향으로 겹친 폭
	FVector contactPoint;
};

// 두 OBB의 겹침을 판정하고, 가장 얕게 겹친 축을 법선으로 돌려준다 (SAT).
SATResult OverlapOBB(const OBB& a, const OBB& b);

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
	float CollisionThreshold = 50.0f;

	// 디버그 표시용. CheckCollisionAll이 반환하는 infos는 데미지 판정을 통과한 것만
	// 담기지만, 이쪽은 이번 프레임에 감지된 접촉 전부를 담는다.
	std::vector<CollisionInfo> debugContacts;

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
