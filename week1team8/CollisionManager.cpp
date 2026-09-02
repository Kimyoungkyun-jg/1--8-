#include <algorithm>

#include "CollisionManager.h"
#include "ObjectManager.h"
#include "GameManager.h"

OBB MakeOBB(const ACollider* collider)
{
	// Rotation은 라디안, 반시계 방향 (UObject.h)
	float angle = collider->GetRotation();
	float cs = std::cos(angle);
	float sn = std::sin(angle);

	OBB box;
	box.center = collider->GetLocation();

	// 월드 x축, y축을 각각 angle만큼 돌린 것이 이 사각형의 로컬 축이다.
	box.axis[0] = FVector(cs, sn, 0.0f);
	box.axis[1] = FVector(-sn, cs, 0.0f);

	box.half[0] = collider->GetScale().x * 0.5f;
	box.half[1] = collider->GetScale().y * 0.5f;

	// 중심에서 두 축 방향으로 반너비/반높이만큼 간 네 점.
	// 회전이 0일 때 좌하 -> 우하 -> 우상 -> 좌상 순서가 되고, 회전해도 순서는 유지된다.
	FVector ex = box.axis[0] * box.half[0];
	FVector ey = box.axis[1] * box.half[1];

	box.vertex[0] = box.center - ex - ey;
	box.vertex[1] = box.center + ex - ey;
	box.vertex[2] = box.center + ex + ey;
	box.vertex[3] = box.center - ex + ey;

	return box;
}

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

float CollisionManager::InvInertia(float inertia)
{
	if (inertia <= 0.0f)
	{
		return 0.0f;
	}

	return 1.0f / inertia;
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
			if (colliders[i]->GetMass() + colliders[j]->GetMass() <= 0.0f)
			{
				continue;
			}

			CollisionInfo info = CheckCollision(colliders[i], colliders[j]);
			info.colAId = colliders[i]->GetColliderId();
			info.colBId = colliders[j]->GetColliderId();

			if (info.isCollision)
			{
				abinfos.push_back({ { colliders[i] , colliders[j] }, info });
			}
		}
	}

	std::sort(abinfos.begin(), abinfos.end(), [](const auto& a, const auto& b) {
		return a.second.contactPoint.y < b.second.contactPoint.y;
		});

	for (auto& [ab, info] : abinfos)
	{
		InitContact(ab.first, ab.second, info);
	}

	for (int i = 0; i < 3; i++)
	{
		for (auto& [ab, info] : abinfos)
		{
			// 충격량(속도) 해결
			SolveContact(ab.first, ab.second, info);
		}
	}

	// 디버그용 스냅샷. 충격량이 다 풀린 뒤라 normalImpulse가 최종값이다.
	debugContacts.clear();
	for (auto& [ab, info] : abinfos)
	{
		debugContacts.push_back(info);
	}

	for (int i = 0; i < 3; i++)
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

	// infos 채우기
	bool bCanDamage = false;
	float MinDamageSpeed = 0.1f;
	if (ABird* Bird = GameManager::GetInstance().GetReloadedBird())
	{
		bCanDamage = (Bird->State == EBirdState::Shooting);
	}

	auto TryKill = [&](ACollider* c)
		{
			if (!c || c->GetMass() <= 0.0f) return;     // 정적 물체(바닥)는 제외
			if (c->minusHp() != 0) return;              // 아직 안 죽음
			for (auto* p : pendingkills)                // 중복 방지
			{
				if (p == c) return;
			}
			pendingkills.push_back(c);
		};

	for (auto& [ab, info] : abinfos)
	{
		if (info.initialNormalVelocity > -MinDamageSpeed) continue;  // 충분히 빠르게 부딪혔나
		if (info.normalImpulse <= CollisionThreshold)    continue;   // 충분히 셌나

		infos.push_back(info);

		if (!bCanDamage) continue;

		TryKill(ab.first);
		TryKill(ab.second);
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

	// 접촉점을 실제로 겹치는 구간 안으로 옮긴다
	if (overlapX < overlapY)
	{
		// 법선이 x축 -> 접촉면은 세로선. y를 겹치는 구간의 중앙으로
		float overlapDown = std::fmax(downA, downB);
		float overlapUp = std::fmin(upA, upB);
		contactPoint.y = (overlapDown + overlapUp) / 2;
	}
	else
	{
		// 법선이 y축 -> 접촉면은 가로선. x를 겹치는 구간의 중앙으로
		float overlapLeft = std::fmax(leftA, leftB);
		float overlapRight = std::fmin(rightA, rightB);
		contactPoint.x = (overlapLeft + overlapRight) / 2;
	}

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

// 충돌해결 (법선 B->A)
void CollisionManager::SolveContact(ACollider* a, ACollider* b, CollisionInfo& info)
{
	FVector rA = info.rA;
	FVector rB = info.rB;
	FVector normal = info.normal;

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	FVector relativeVelocity = (a->GetVelocity() + FVector::Cross(a->GetAngularVelocity(), rA)) -
		(b->GetVelocity() + FVector::Cross(b->GetAngularVelocity(), rB)); // 상대 속도
	float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

	// 충격량 적용
	float raxn = FVector::Cross(rA, normal);
	float rbxn = FVector::Cross(rB, normal);

	float delta = -(relativeVelocityNormal - info.velocityBias) * info.normalMass;
	float oldImpulse = info.normalImpulse;
	info.normalImpulse = std::fmax(oldImpulse + delta, 0.0f);
	float applied = info.normalImpulse - oldImpulse;

	a->SetVelocity(a->GetVelocity() + normal * (applied * invMassA));
	b->SetVelocity(b->GetVelocity() - normal * (applied * invMassB));
	a->SetAngularVelocity(a->GetAngularVelocity() + raxn * applied * invIA);
	b->SetAngularVelocity(b->GetAngularVelocity() - rbxn * applied * invIB);

	// 마찰 적용
	FVector relativeVelocityAfter = (a->GetVelocity() + FVector::Cross(a->GetAngularVelocity(), rA)) -
		(b->GetVelocity() + FVector::Cross(b->GetAngularVelocity(), rB));

	float raxt = FVector::Cross(rA, info.tangent);
	float rbxt = FVector::Cross(rB, info.tangent);

	float vt = info.tangent.DotProduct(relativeVelocityAfter);   // 음수 가능
	float deltaT = -vt * info.tangentMass;                        // 여기도 곱셈
	float oldT = info.tangentImpulse;
	float newT = oldT + deltaT;

	float staticFriction = std::sqrt(a->GetStaticFriction() * b->GetStaticFriction()); // 정지 마찰 계수
	float maxStaticFriction = info.normalImpulse * staticFriction;

	if (std::fabs(newT) > maxStaticFriction)
	{
		// 정지 마찰 한계를 넘음 -> 미끄러짐, 운동 마찰로 클램프
		float dynamicFriction = std::sqrt(a->GetDynamicFriction() * b->GetDynamicFriction());
		float maxDynamicFriction = info.normalImpulse * dynamicFriction;
		newT = std::clamp(newT, -maxDynamicFriction, maxDynamicFriction);
	}
	// else: 정지 마찰 범위 안 -> frictionImpulseMag 그대로 적용, 즉 붙잡혀서 안 미끄러짐

	info.tangentImpulse = newT;
	float appliedT = newT - oldT;

	a->SetVelocity(a->GetVelocity() + info.tangent * (appliedT * invMassA));
	b->SetVelocity(b->GetVelocity() - info.tangent * (appliedT * invMassB));
	a->SetAngularVelocity(a->GetAngularVelocity() + raxt * appliedT * invIA);
	b->SetAngularVelocity(b->GetAngularVelocity() - rbxt * appliedT * invIB);
}

void CollisionManager::InitContact(ACollider* a, ACollider* b, CollisionInfo& info)
{
	info.normalImpulse = 0.0f;
	info.tangentImpulse = 0.0f;
	info.normalMass = 0.0f;
	info.tangentMass = 0.0f;

	FVector rA = info.contactPoint - a->GetLocation();
	FVector rB = info.contactPoint - b->GetLocation();

	FVector normal = info.normal;

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	if (invMassA + invMassB <= 0.0f) return; // 스태틱 충돌

	FVector relativeVelocity = (a->GetVelocity() + FVector::Cross(a->GetAngularVelocity(), rA)) -
		(b->GetVelocity() + FVector::Cross(b->GetAngularVelocity(), rB)); // 상대 속도
	float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

	const float restitutionThreshold = 1.0f; // 튜닝 값
	float restitution = (std::fabs(relativeVelocityNormal) < restitutionThreshold) ? 0.0f : 0.2f;

	// 충격량
	float raxn = FVector::Cross(rA, normal);
	float rbxn = FVector::Cross(rB, normal);
	float validMass = invMassA + invMassB + raxn * raxn * invIA + rbxn * rbxn * invIB;

	info.tangent = FVector::Cross(normal, 1.0f);

	float raxt = FVector::Cross(rA, info.tangent);
	float rbxt = FVector::Cross(rB, info.tangent);
	float validMassTangent = invMassA + invMassB + raxt * raxt * invIA + rbxt * rbxt * invIB;

	info.rA = rA;
	info.rB = rB;
	info.normalMass = 1.0f / validMass;
	info.tangentMass = 1.0f / validMassTangent;
	info.initialNormalVelocity = relativeVelocityNormal;
	info.velocityBias = (relativeVelocityNormal < -restitutionThreshold) ? -restitution * relativeVelocityNormal : 0.0f;
}
