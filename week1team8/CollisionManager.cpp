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

namespace
{
	// box의 네 꼭짓점을 축 dir에 투영했을 때의 [min, max] 구간
	void ProjectOBB(const OBB& box, const FVector& dir, float& outMin, float& outMax)
	{
		outMin = outMax = box.vertex[0].DotProduct(dir);

		for (int i = 1; i < 4; i++)
		{
			float d = box.vertex[i].DotProduct(dir);
			outMin = std::fmin(outMin, d);
			outMax = std::fmax(outMax, d);
		}
	}

	// dir 방향으로 가장 멀리 나간 꼭짓점의 번호
	int SupportVertex(const OBB& box, const FVector& dir)
	{
		int best = 0;
		float bestDot = box.vertex[0].DotProduct(dir);

		for (int i = 1; i < 4; i++)
		{
			float d = box.vertex[i].DotProduct(dir);
			if (d > bestDot)
			{
				bestDot = d;
				best = i;
			}
		}

		return best;
	}
}

SATResult OverlapOBB(const OBB& a, const OBB& b)
{
	SATResult result;

	// 볼록 도형 둘이 안 겹치면, 둘을 갈라놓는 축이 반드시 하나 있다 (분리축 정리).
	// 사각형에서 후보가 되는 축은 각 상자의 로컬 축 2개씩, 총 4개다.
	// 마주보는 두 면은 방향만 반대라 축으로는 같으므로 면 8개를 다 볼 필요가 없다.
	FVector axes[4] = { a.axis[0], a.axis[1], b.axis[0], b.axis[1] };

	float minOverlap = FLT_MAX;
	int minIndex = 0;

	for (int i = 0; i < 4; i++)
	{
		float aMin, aMax, bMin, bMax;
		ProjectOBB(a, axes[i], aMin, aMax);
		ProjectOBB(b, axes[i], bMin, bMax);

		// 이 축에서 두 구간이 떨어져 있으면 분리축을 찾은 것 -> 안 겹친다
		if (aMax <= bMin || bMax <= aMin)
		{
			return result;
		}

		// 두 구간이 겹치는 폭
		float overlap = std::fmin(aMax, bMax) - std::fmax(aMin, bMin);

		if (overlap < minOverlap)
		{
			minOverlap = overlap;
			minIndex = i;
		}
	}

	// 겹침이 가장 얕은 축으로 밀어내는 게 가장 적게 움직이고 빠져나가는 길이다.
	// 축의 부호는 임의라, a가 b의 반대편으로 가도록(B -> A) 맞춰준다.
	FVector normal = axes[minIndex];
	if ((a.center - b.center).DotProduct(normal) < 0.0f)
	{
		normal = normal * -1.0f;
	}

	// 최소 침투 축이 어느 상자의 것이었나에 따라, 그 상자의 면이 '기준면'이고
	// 반대쪽 상자의 꼭짓점이 그 면을 파고든 점이다. (5단계 클리핑의 출발점)
	// 접촉점은 아직 하나뿐이라 그 꼭짓점을 그대로 쓴다.
	if (minIndex < 2)
	{
		// 기준면이 a에 있음 -> 파고든 건 b의 꼭짓점. a 쪽(+normal)으로 가장 멀리 간 점.
		result.contactPoint = b.vertex[SupportVertex(b, normal)];
	}
	else
	{
		// 기준면이 b에 있음 -> 파고든 건 a의 꼭짓점. b 쪽(-normal)으로 가장 멀리 간 점.
		result.contactPoint = a.vertex[SupportVertex(a, normal * -1.0f)];
	}

	result.overlapped = true;
	result.normal = normal;
	result.penetration = minOverlap;

	return result;
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
	SATResult result = OverlapOBB(MakeOBB(a), MakeOBB(b));

	if (!result.overlapped)
	{
		return CollisionInfo();
	}

	// 매우 작은 겹침 무시
	if (result.penetration <= 0.0001f)
	{
		return CollisionInfo();
	}

	CollisionInfo info
	{
		result.contactPoint,
		result.normal,
		result.penetration,
		true
	};


	return info;
}

// a == Circle, b == Rectangle
CollisionInfo CollisionManager::CheckCollisionCircleRectangle(ACollider* a, ACollider* b)
{
	OBB box = MakeOBB(b);
	float radius = a->GetScale().x / 2;

	// 원 중심을 사각형의 로컬 좌표계로 옮긴다.
	// 축에 내적하면 '그 축 방향으로 얼마나 갔는지'가 나오고, 이 좌표계에서는
	// 사각형이 축 정렬 상태가 되므로 회전을 신경 쓸 필요가 없어진다.
	FVector toCenter = a->GetLocation() - box.center;
	float localX = toCenter.DotProduct(box.axis[0]);
	float localY = toCenter.DotProduct(box.axis[1]);

	// 사각형 안에서 원 중심과 가장 가까운 점 (로컬 좌표)
	float clampedX = std::clamp(localX, -box.half[0], box.half[0]);
	float clampedY = std::clamp(localY, -box.half[1], box.half[1]);

	// 다시 월드 좌표로. 중심에서 두 축 방향으로 그만큼 간 점이다.
	FVector closest = box.center + box.axis[0] * clampedX + box.axis[1] * clampedY;

	FVector diff = a->GetLocation() - closest;
	float dist = diff.Length();

	bool isCollision = dist < radius;

	// 원이 사각형 내부에 들어감 (추후 수정)
	if (dist <= 0.0001f)
	{
		return CollisionInfo();
	}

	// 충돌 법선 단위 벡터
	FVector normal = diff;
	normal.Normalize();

	// 침투
	float penetration = radius - dist;

	// 충돌 지점
	FVector pointA = a->GetLocation() - normal * radius;
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
