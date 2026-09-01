#include "CollisionManager.h"

CollisionManager& CollisionManager::GetInstance() // 싱글톤 패턴으로 관리
{
	static CollisionManager instance;
	return instance;
}

CollisionManager::CollisionManager() {}

CollisionManager::~CollisionManager() {}

float CollisionManager::InvMass(float mass)
{
	if (mass <= 0.0f)
	{
		return 0.0f;
	}

	return 1.0f / mass;
}

std::vector<CollisionManager::CollisionInfo> CollisionManager::CheckCollisionAll()
{
	std::vector<CollisionInfo> infos;
	size_t n = colliders.size();

	for (size_t i = 0; i < n; i++)
	{
		for (size_t j = i + 1; j < n; j++)
		{
			CollisionInfo info = CheckCollision(colliders[i], colliders[j]);
			info.a = colliders[i];
			info.b = colliders[j];
			infos.push_back(info);

			if (info.isCollision)
			{
				ResolveCollision(colliders[i], colliders[j], info);
			}
		}
	}

	return infos;
}

// 충돌 감지
CollisionManager::CollisionInfo CollisionManager::CheckCollision(ACollider* a, ACollider* b)
{
	if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Circle)
	{
		return CheckCollisionCircleCircle(a, b);
	}
	else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Rectangle)
	{
		return CheckCollisionRectangleRectangle(a, b);
	}
	else if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Rectangle)
	{
		return CheckCollisionCircleRectangle(a, b);
	}
	else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Circle)
	{
		CollisionInfo info = CheckCollisionCircleRectangle(b, a);

		info.normal = info.normal * -1.0f;

		return info;
	}

	return CollisionInfo();
}

// 위치, 크기(가로, 세로, 반지름), 회전, 속도, 무게 필요
CollisionManager::CollisionInfo CollisionManager::CheckCollisionCircleCircle(ACollider* a, ACollider* b)
{
	// 충돌 감지
	FVector diff = a->GetLocation() - b->GetLocation();
	float dist = diff.Length();
	float radiusSum = (a->GetScale().x / 2 + b->GetScale().x / 2);
	bool isCollision = dist < radiusSum;

	// 중심이 매우 겹침 (추후 수정)
	if (dist <= 0.0001f || !isCollision)
	{
		return CollisionInfo();
	}

	// 충돌 법선 단위 벡터
	FVector normal = diff;
	normal.Normalize();

	// 침투
	float penetration = radiusSum - dist;

	// 충돌 지점
	FVector pointA = a->GetLocation() - normal * a->GetScale().x / 2;
	FVector pointB = b->GetLocation() + normal * b->GetScale().x / 2;
	FVector contactPoint = (pointA + pointB) / 2;

	CollisionInfo info
	{
		contactPoint,
		normal,
		penetration,
		isCollision
	};

	return info;
}

CollisionManager::CollisionInfo CollisionManager::CheckCollisionRectangleRectangle(ACollider* a, ACollider* b)
{
	// 충돌 감지
	float widthA = a->GetScale().x;
	float heightA = a->GetScale().y;

	float widthB = b->GetScale().x;
	float heightB = b->GetScale().y;

	float leftA = a->GetLocation().x - widthA / 2;
	float rightA = a->GetLocation().x + widthA / 2;
	float upA = a->GetLocation().y + heightA / 2;
	float downA = a->GetLocation().y - heightA / 2;

	float leftB = b->GetLocation().x - widthB / 2;
	float rightB = b->GetLocation().x + widthB / 2;
	float upB = b->GetLocation().y + heightB / 2;
	float downB = b->GetLocation().y - heightB / 2;

	bool isCollision = !(leftA >= rightB || rightA <= leftB || upA <= downB || downA >= upB);

	if (!isCollision)
	{
		return CollisionInfo();
	}

	float overlapX = (widthA + widthB) / 2 - std::fabs(a->GetLocation().x - b->GetLocation().x);
	float overlapY = (heightA + heightB) / 2 - std::fabs(a->GetLocation().y - b->GetLocation().y);

	// 매우 작은 겹침 무시
	if (overlapX <= 0.0001f || overlapY <= 0.0001f)
	{
		return CollisionInfo();
	}

	// 충돌 법선 단위 벡터
	FVector normal;
	float penetration;
	FVector pointA;
	FVector pointB;

	if (overlapX < overlapY)
	{
		penetration = overlapX;
		if (b->GetLocation().x > a->GetLocation().x)
		{
			normal = FVector(-1.0f, 0.0f, 0.0f);
		}
		else
		{
			normal = FVector(1.0f, 0.0f, 0.0f);
		}
		pointA = a->GetLocation() - normal * a->GetScale().x / 2;
		pointB = b->GetLocation() + normal * b->GetScale().x / 2;
	}
	else
	{
		penetration = overlapY;
		if (b->GetLocation().y > a->GetLocation().y)
		{
			normal = FVector(0.0f, -1.0f, 0.0f);
		}
		else
		{
			normal = FVector(0.0f, 1.0f, 0.0f);
		}
		pointA = a->GetLocation() - normal * a->GetScale().y / 2;
		pointB = b->GetLocation() + normal * b->GetScale().y / 2;
	}

	// 충돌 지점
	FVector contactPoint = (pointA + pointB) / 2;

	CollisionInfo info
	{
		contactPoint,
		normal,
		penetration,
		isCollision
	};

	return info;
}

// a == Circle, b == Rectangle
CollisionManager::CollisionInfo CollisionManager::CheckCollisionCircleRectangle(ACollider* a, ACollider* b)
{
	// 충돌 감지
	float widthB = b->GetScale().x;
	float heightB = b->GetScale().y;

	float leftB = b->GetLocation().x - widthB / 2;
	float rightB = b->GetLocation().x + widthB / 2;
	float upB = b->GetLocation().y + heightB / 2;
	float downB = b->GetLocation().y - heightB / 2;

	float x = std::clamp(a->GetLocation().x, leftB, rightB);
	float y = std::clamp(a->GetLocation().y, downB, upB);

	// 사각형 내부의 점 중 원과 가장 가까운 점
	FVector closest(x, y);
	FVector diff = a->GetLocation() - closest;
	float dist = diff.Length();

	bool isCollision = dist < a->GetScale().x / 2;

	// 원이 사각형 내부에 들어감
	if (dist <= 0.0001f)
	{
		return CollisionInfo();
	}

	// 충돌 법선 단위 벡터
	FVector normal = diff;
	normal.Normalize();

	// 침투
	float penetration = a->GetScale().x / 2 - dist;

	// 충돌 지점
	FVector pointA = a->GetLocation() - normal * a->GetScale().x / 2;
	FVector pointB = closest;
	FVector contactPoint = (pointA + pointB) / 2;

	CollisionInfo info
	{
		contactPoint,
		normal,
		penetration,
		isCollision
	};

	return info;
}

// 충돌해결
void CollisionManager::ResolveCollision(ACollider* a, ACollider* b, const CollisionInfo& info)
{
	FVector normal = info.normal;
	float penetration = info.penetration;

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());

	// 스태틱 충돌 (추후 수정)
	if (invMassA + invMassB <= 0.0f)
	{
		return;
	}

	// 위치 보정
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
