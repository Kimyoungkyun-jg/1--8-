#include <algorithm>

#include "CollisionManager.h"
#include "ObjectManager.h"

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

void CollisionManager::SetAllCollisionFriction(float _dynamic, float _static)
{
	size_t n = colliders.size();
	for (size_t i = 0; i < n; i++)
	{
		colliders[i]->SetdynamicFriction(_dynamic);
		colliders[i]->SetStaticFriction(_static);
	}
}

std::vector<CollisionInfo> CollisionManager::CheckCollisionAll()
{
	std::vector<CollisionInfo> infos;
	std::vector<std::pair<std::pair<ACollider*, ACollider*>, CollisionInfo>> abinfos;
	size_t n = colliders.size();


	for (size_t i = 0; i < n; i++)
	{
		for (size_t j = i + 1; j < n; j++)
		{
			CollisionInfo info = CheckCollision(colliders[i], colliders[j]);
			info.colAId = colliders[i]->GetColliderId();
			info.colBId = colliders[j]->GetColliderId();

			if (info.isCollision)
			{
				float impulse = ResolveCollision(colliders[i], colliders[j], info);
				if (impulse > 10.0f)
				{
					infos.push_back(info);
				}
				abinfos.push_back({ { colliders[i] , colliders[j] }, info });
			}
		}
	}

	std::sort(abinfos.begin(), abinfos.end(), [](const auto& a, const auto& b) {
		return a.second.contactPoint.y < b.second.contactPoint.y;
		});

	for (int i = 0; i < 10; i++)
	{
		for (auto& [ab, info] : abinfos)
		{
			// 충격량(속도) 해결
			ResolveCollision(ab.first, ab.second, info);
		}
	}

	for (int i = 0; i < 20; i++)
	{
		for (auto& [ab, info] : abinfos)
		{
			// 앞선 루프나 충격량 처리에 의해 위치가 변경되었으므로,
			// 현재 위치를 기준으로 겹침(penetration)을 '다시' 계산해야 합니다.
			CollisionInfo currentInfo = CheckCollision(ab.first, ab.second);

			if (currentInfo.isCollision)
			{
				// 새롭게 계산된 정보(currentInfo)로 위치 보정
				ResolvePosition(ab.first, ab.second, currentInfo);
			}
		}
	}

	for (int i = (int)pendingkills.size() - 1; i >= 0; i--)
	{
		UObjectManager::GetInstance().Destroy(pendingkills[i]);
	}

	pendingkills.clear();


	return infos;
}

// 충돌 감지
CollisionInfo CollisionManager::CheckCollision(ACollider* a, ACollider* b)
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
CollisionInfo CollisionManager::CheckCollisionCircleCircle(ACollider* a, ACollider* b)
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

CollisionInfo CollisionManager::CheckCollisionRectangleRectangle(ACollider* a, ACollider* b)
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
CollisionInfo CollisionManager::CheckCollisionCircleRectangle(ACollider* a, ACollider* b)
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

	// 원이 사각형 내부에 들어감 (추후 수정)
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

void CollisionManager::ResolvePosition(ACollider* a, ACollider* b, const CollisionInfo& info)
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
	const float slop = 0.001f; // 이 정도 침투는 무시
	const float baumgarte = 0.5f;   // 나중에 dt 기반으로 변경

	float correctionAmount = std::fmax(penetration - slop, 0.0f);
	if (correctionAmount > 0.0f)
	{
		FVector correction = normal * correctionAmount * baumgarte / (invMassA + invMassB);
		a->SetLocation(a->GetLocation() + correction * invMassA);
		b->SetLocation(b->GetLocation() - correction * invMassB);
	}
}

// 충돌해결
float CollisionManager::ResolveCollision(ACollider* a, ACollider* b, const CollisionInfo& info)
{
	FVector normal = info.normal;
	float penetration = info.penetration;

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());

	// 스태틱 충돌 (추후 수정)
	if (invMassA + invMassB <= 0.0f)
	{
		return 0.0f;
	}

	FVector relativeVelocity = a->GetVelocity() - b->GetVelocity(); // 상대 속도
	float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

	// 충돌 지점에서 멀어지는 중 (내적의 결과가 양수 = 충돌 방향과 상대 속도가 예각을 이룸
	if (relativeVelocityNormal >= 0)
	{
		return 0.0f;
	}

	const float restitutionThreshold = 1.0f; // 튜닝 값
	float restitution = (std::fabs(relativeVelocityNormal) < restitutionThreshold) ? 0.0f : 0.8f;

	// 충격량 적용
	float impulseMag = -(1.0f + restitution) * relativeVelocityNormal / (invMassA + invMassB); // 충격량
	a->SetVelocity(a->GetVelocity() + normal * (impulseMag * invMassA));
	b->SetVelocity(b->GetVelocity() - normal * (impulseMag * invMassB));

	// 마찰 적용
	FVector relativeVelocityAfter = a->GetVelocity() - b->GetVelocity();
	FVector tangent = relativeVelocityAfter - normal * normal.DotProduct(relativeVelocityAfter); // 접선 방향 상대 속도
	float tangentLength = tangent.Length(); // 속력

	if (tangentLength > 0.0001f)
	{
		tangent = tangent / tangentLength; // 정규화

		float relativeVelocityTangent = tangentLength; // 접선 방향 속력
		float frictionImpulseMag = -relativeVelocityTangent / (invMassA + invMassB); // 마찰 임펄스 크기

		float staticFriction = std::sqrt(a->GetStaticFriction() * b->GetStaticFriction()); // 정지 마찰 계수
		float maxStaticFriction = impulseMag * staticFriction;

		if (std::fabs(frictionImpulseMag) > maxStaticFriction)
		{
			// 정지 마찰 한계를 넘음 -> 미끄러짐, 운동 마찰로 클램프
			float dynamicFriction = std::sqrt(a->GetDynamicFriction() * b->GetDynamicFriction());
			float maxDynamicFriction = impulseMag * dynamicFriction;
			frictionImpulseMag = std::clamp(frictionImpulseMag, -maxDynamicFriction, maxDynamicFriction);
		}
		// else: 정지 마찰 범위 안 -> frictionImpulseMag 그대로 적용, 즉 붙잡혀서 안 미끄러짐

		a->SetVelocity(a->GetVelocity() + tangent * (frictionImpulseMag * invMassA));
		b->SetVelocity(b->GetVelocity() - tangent * (frictionImpulseMag * invMassB));
	}

	if (impulseMag > 80.0f)
	{
		AObstacle* oba = dynamic_cast<AObstacle*>(a);
		AObstacle* obb = dynamic_cast<AObstacle*>(b);

		if (oba)
		{
			if (oba->minusHp() == 0)
			{
				bool alreadyPending = false;
				for (auto* p : pendingkills)
				{
					if (p == oba) { alreadyPending = true; break; }
				}
				if (!alreadyPending)
				{
					pendingkills.push_back(oba);
				}
			}
		}

		if (obb)
		{
			if (obb->minusHp() == 0)
			{
				bool alreadyPending = false;
				for (auto* p : pendingkills)
				{
					if (p == obb) { alreadyPending = true; break; }
				}
				if (!alreadyPending)
				{
					pendingkills.push_back(obb);
				}
			}
		}
	}

	return impulseMag;
}
