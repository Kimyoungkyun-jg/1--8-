#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <unordered_map>

#include "UObject.h"
#include "SoundManager.h"


// 접촉점
struct ContactPoint
{
	FVector position = FVector();
	float penetration = 0.0f;
	unsigned int id = 0;         // 어느 면·꼭짓점에서 나왔는지. warm starting이 이걸로 짝을 찾음

	FVector rA, rB;              // 질량중심 -> 접촉점
	FVector localA, localB;      // 각 물체 로컬 좌표의 접촉점. 위치 보정이 깊이를 다시 잴 때 쓴다
	float normalMass = 0.0f;     // 1 / validMass       (미리 나눠둔 값)
	float tangentMass = 0.0f;    // 1 / validMassTangent
	float velocityBias = 0.0f;   // 목표 분리 속도

	float normalImpulse = 0.0f;  // 누적 충격량 1: 법선
	float tangentImpulse = 0.0f; // 누적 충격량 2: 마찰
	float rollingImpulse = 0.0f; // 누적 충격량 3: 구름 저항 (각충격량)
	float initialNormalVelocity = 0.0f;

	float inheritedNormalImpulse = 0.0f;   // 이월받은 몫 = 지지 하중. 빼면 이번 충돌분만 남는다
};


struct CollisionInfo
{
	FVector normal = FVector();
	FVector tangent = FVector();
	float staticFriction = 0.0f;    // 쌍이 정하는 값이라 접촉점마다 같다
	float dynamicFriction = 0.0f;
	bool isCollision = false;
	bool bNewContact = false;    // 지난 프레임엔 안 닿아 있었다 = 이번에 새로 부딪힘
	EColliderId colAId = EColliderId::NONE;
	EColliderId colBId = EColliderId::NONE;
	int bodyA = 0;               // 디버그에서 물체 목록과 접촉 목록을 연결하는 용도
	int bodyB = 0;

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

// 회전을 포함한 사각형. 면 i는 vertex[i] -> vertex[i+1], 바깥 방향이 normal[i]
struct OBB
{
	FVector center;
	FVector axis[2];    // 회전한 로컬 x축, y축 (단위벡터)
	float half[2];      // 반너비, 반높이
	FVector vertex[4];  // 반시계 방향: 좌하, 우하, 우상, 좌상
	FVector normal[4];  // 각 면의 바깥 방향
};

OBB MakeOBB(const ACollider* collider);

// SAT 판정 결과. 면끼리 닿으면 접촉점이 2개
struct SATContact
{
	FVector position;
	float penetration = 0.0f;   // 이 점이 기준면을 파고든 깊이
	unsigned int id = 0;        // 프레임 간 추적용
};

struct SATResult
{
	bool overlapped = false;
	FVector normal;             // B -> A 방향 (a를 밀어낼 방향)
	int pointCount = 0;
	SATContact points[2];
};

// 두 OBB의 겹침을 판정하고, 가장 얕게 겹친 축을 법선으로 돌려줌 (SAT).
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

	void TryKill(ACollider * c)
	{
		if (!c || c->GetMass() <= 0.0f) return;     // 정적 물체(바닥)는 제외
		c->PlaySFX();
		if (c->minusHp() > 0) return;              // 아직 안 죽음
		for (auto* p : pendingkills)                // 중복 방지
		{
			if (p == c) return;
		}
		pendingkills.push_back(c);
	};

	// 디버그용. 반환값 infos와 달리 데미지 판정 전의 모든 접촉을 담는다
	std::vector<CollisionInfo> debugContacts;

	// 위치 보정 후 남은 가장 깊은 겹침. slop 근처면 보정이 제 일을 다 한 것
	float maxPenetration = 0.0f;

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

	// 둘 다 못 움직이면 풀 필요가 없다 (정적끼리, 잠든 것끼리, 정적-잠듦)
	static bool IsPairSolvable(const ACollider* a, const ACollider* b)
	{
		return (a->GetMass() > 0.0f && !a->IsSleeping())
			|| (b->GetMass() > 0.0f && !b->IsSleeping());
	}

	std::vector<CollisionInfo> CheckCollisionAll(float t);

	// 무리(island) 단위로 재운다. 하나라도 타이머를 못 채우면 무리 전체가 깨어 있는다
	void UpdateSleep(float t, const std::vector<std::pair<ACollider*, ACollider*>>& contacts);

	// 타이머만 채워서 바로 잠들 준비를 시킨다 (맵 로드 직후용). 뜬 물체는 그대로 떨어진다
	void PrimeSleep();

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
	void WarmStartContact(ACollider* a, ACollider* b, const CollisionInfo& info);

	bool bWarmStarting = true;   // 끄고 켜서 효과 비교용

	// 솔버 튜닝 값 (Physics Debug에서 실시간 조절)
	int velocityIterations = 8;   // 충격량이 접촉을 타고 전파되는 횟수
	int positionIterations = 15;
	float baumgarte = 0.8f;      // 겹침을 한 번에 얼마나 밀어낼지
	float slop = 0.0005f;         // 이 정도 침투는 무시

	float rollingResistance = 0.002f;   // 구름 저항의 팔 길이. 무차원이 아니라 길이 단위다

	// 슬립
	bool bSleepEnabled = true;
	float linearSleepTolerance = 0.015f;
	float angularSleepTolerance = 0.035f;   // 약 2도/초
	float timeToSleep = 0.5f;

private:
	// 지난 프레임의 접촉들. warm starting이 여기서 충격량을 찾아 이월한다
	std::unordered_map<unsigned long long, CollisionInfo> previousManifolds;

	static unsigned long long MakePairKey(const ACollider* a, const ACollider* b);
	const CollisionInfo* FindPreviousManifold(const ACollider* a, const ACollider* b) const;
};
