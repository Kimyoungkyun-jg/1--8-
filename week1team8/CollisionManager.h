#pragma once

#include <vector>
#include <cmath>

#include "UObject.h"

class CollisionManager
{
public:
	static CollisionManager& GetInstance() // 싱글톤 패턴으로 관리
	{
		static CollisionManager instance;
		return instance;
	}

	CollisionManager(const CollisionManager&) = delete;
	CollisionManager& operator=(const CollisionManager&) = delete;
	CollisionManager() {};
	~CollisionManager() {};

	std::vector<ACollider*> colliders;

	void CheckCollisionAll()
	{
		int n = colliders.size();

		for (int i = 0; i < n; i++)
		{
			for (int j = i + 1; j < n; j++)
			{
				CheckCollision(colliders[i], colliders[j]);
			}
		}
	}

	// 충돌 감지 및 해결
	void CheckCollision(ACollider* a, ACollider* b)
	{
		if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Circle)
		{
			CheckCollisionCircleCircle(a, b);
		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Rectangle)
		{
			CheckCollisionRectangleRectangle(a, b);
		}
		else if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Rectangle)
		{
			CheckCollisionCircleRectangle(a, b);
		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Circle)
		{
			CheckCollisionCircleRectangle(b, a);
		}
	}

	// 위치, 크기(가로, 세로, 반지름), 회전, 속도, 무게 필요
	void CheckCollisionCircleCircle(ACollider* a, ACollider* b)
	{
		// 충돌 감지
		FVector diff = a->GetLocation() - b->GetLocation();
		float dist = diff.Length();
		float radiusSum = (a->GetScale().x + b->GetScale().x);
		bool isCollision = dist < radiusSum;

		// 충돌 없음 || 중심이 매우 겹침
		if (!isCollision || dist <= 0.0001f)
		{
			return;
		}

		// 충돌 법선 단위 벡터
		FVector normal = diff;
		normal.Normalize();

		// 겹침 해결
		float penetration = radiusSum - dist;
		float invMassA = 1.0f / a->GetMass();
		float invMassB = 1.0f / b->GetMass();

		if (penetration <= 0.0001f) // 매우 작은 겹침 무시
		{
			return;
		}

		FVector correction = normal * penetration / (invMassA + invMassB);
		a->SetLocation(a->GetLocation() + correction * invMassA);
		b->SetLocation(b->GetLocation() - correction * invMassB);

		FVector relativeVelocity = a->GetVelocity() - b->GetVelocity(); // 상대 속도
		float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

		// 충돌 지점에서 멀어지는 중 (내적의 결과가 양수 = 충돌 방향과 상대 속도가 예각을 이룸
		if (relativeVelocityNormal >= 0)
		{
			return;
		}

		float restitution = 0.8f; // 반발계수
		float impulse = -(1.0f + restitution) * relativeVelocityNormal / (invMassA + invMassB); // 충격량

		// 충격량 적용
		a->SetVelocity(a->GetVelocity() + normal * (impulse * invMassA));
		b->SetVelocity(b->GetVelocity() - normal * (impulse * invMassB));
	}

	void CheckCollisionRectangleRectangle(ACollider* a, ACollider* b)
	{
		float widthA = a->GetScale().x;
		float heightA = a->GetScale().y;

		float widthB = b->GetScale().x;
		float heightB = b->GetScale().y;

		// 충돌 감지
		float leftA = a->GetLocation().x - widthA / 2;
		float rightA = a->GetLocation().x + widthA / 2;
		float upA = a->GetLocation().y + heightA / 2;
		float downA = a->GetLocation().y - heightA / 2;

		float leftB = b->GetLocation().x - widthB / 2;
		float rightB = b->GetLocation().x + widthB / 2;
		float upB = b->GetLocation().y + heightB / 2;
		float downB = b->GetLocation().y - heightB / 2;

		bool isCollision = !(leftA >= rightB || rightA <= leftB || upA <= downB || downA >= upB);

		// 충돌 없음
		if (!isCollision)
		{
			return;
		}

		float overlapX = (widthA + widthB) / 2 - std::fabs(a->GetLocation().x - b->GetLocation().x);
		float overlapY = (heightA + heightB) / 2 - std::fabs(a->GetLocation().y - b->GetLocation().y);

		// 매우 작은 겹침 무시
		if (overlapX <= 0.0001f || overlapY <= 0.0001f)
		{
			return;
		}

		// 충돌 법선 단위 벡터
		FVector normal;
		if (overlapX < overlapY)
		{
			if (b->GetLocation().x > a->GetLocation().x)
			{
				normal = FVector(-1.0f, 0.0f, 0.0f);
			}
			else
			{
				normal = FVector(1.0f, 0.0f, 0.0f);
			}
		}
		else
		{
			if (b->GetLocation().y > a->GetLocation().y)
			{
				normal = FVector(0.0f, -1.0f, 0.0f);
			}
			else
			{
				normal = FVector(0.0f, 1.0f, 0.0f);
			}
		}

		// 겹침 해결
		float penetration = std::fmin(overlapX, overlapY);
		float invMassA = 1.0f / a->GetMass();
		float invMassB = 1.0f / b->GetMass();

		FVector correction = normal * penetration / (invMassA + invMassB);
		a->SetLocation(a->GetLocation() + correction * invMassA);
		b->SetLocation(b->GetLocation() - correction * invMassB);

		FVector relativeVelocity = a->GetVelocity() - b->GetVelocity(); // 상대 속도
		float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

		// 충돌 지점에서 멀어지는 중 (내적의 결과가 양수 = 충돌 방향과 상대 속도가 예각을 이룸
		if (relativeVelocityNormal >= 0)
		{
			return;
		}

		float restitution = 0.8f; // 반발계수
		float impulse = -(1.0f + restitution) * relativeVelocityNormal / (invMassA + invMassB); // 충격량

		// 충격량 적용
		a->SetVelocity(a->GetVelocity() + normal * (impulse * invMassA));
		b->SetVelocity(b->GetVelocity() - normal * (impulse * invMassB));
	}

	void CheckCollisionCircleRectangle(ACollider* a, ACollider* b)
	{

	}
};
