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

	// 면 i는 vertex[i] -> vertex[i+1]. 꼭짓점이 반시계 순서라 바깥 방향이 이렇게 정해진다.
	box.normal[0] = box.axis[1] * -1.0f;   // 아래 (v0 -> v1)
	box.normal[1] = box.axis[0];           // 오른쪽 (v1 -> v2)
	box.normal[2] = box.axis[1];           // 위 (v2 -> v3)
	box.normal[3] = box.axis[0] * -1.0f;   // 왼쪽 (v3 -> v0)

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

	// box의 면 중 dir과 가장 잘 맞는(내적이 가장 큰) 면의 번호
	int BestFace(const OBB& box, const FVector& dir)
	{
		int best = 0;
		float bestDot = box.normal[0].DotProduct(dir);

		for (int i = 1; i < 4; i++)
		{
			float d = box.normal[i].DotProduct(dir);
			if (d > bestDot)
			{
				bestDot = d;
				best = i;
			}
		}

		return best;
	}

	// 클리핑을 거치는 동안 '어디서 나온 점인지'를 위치와 함께 들고 다닌다
	struct ClipVertex
	{
		FVector position;
		unsigned int id = 0;
	};

	// 접촉점의 출처를 정수 하나로 압축한다.
	// 기준면 / 상대면 / 어느 꼭짓점(또는 어느 옆면에서 잘렸는지) / 기준이 뒤집혔는지.
	// 이 넷이 같으면 프레임이 넘어가도 물리적으로 같은 접촉이다.
	unsigned int MakeContactId(int referenceFace, int incidentFace, int feature, bool flip)
	{
		return (unsigned int)referenceFace
			| ((unsigned int)incidentFace << 8)
			| ((unsigned int)feature << 16)
			| ((unsigned int)(flip ? 1 : 0) << 24);
	}

	// 선분 in을 반평면 (n·x <= offset) 안쪽만 남기고 자른다.
	// 밖으로 나간 끝점은 평면과 만나는 지점으로 옮겨지고, 그 점은 clipId를 갖는다.
	int ClipSegment(ClipVertex out[2], const ClipVertex in[2], const FVector& n, float offset, unsigned int clipId)
	{
		int count = 0;

		// 평면 기준 부호 있는 거리. 음수면 안쪽.
		float d0 = n.DotProduct(in[0].position) - offset;
		float d1 = n.DotProduct(in[1].position) - offset;

		if (d0 <= 0.0f) out[count++] = in[0];
		if (d1 <= 0.0f) out[count++] = in[1];

		// 한쪽만 밖에 있으면 선분이 평면을 가로지른다. 그 교점을 추가한다.
		if (d0 * d1 < 0.0f && count < 2)
		{
			float t = d0 / (d0 - d1);
			out[count].position = in[0].position + (in[1].position - in[0].position) * t;
			out[count].id = clipId;
			count++;
		}

		return count;
	}
}

SATResult OverlapOBB(const OBB& a, const OBB& b)
{
	SATResult result;

	// 볼록 도형 둘이 안 겹치면, 둘을 갈라놓는 축이 반드시 하나 있다 (분리축 정리).
	// 사각형에서 후보가 되는 축은 각 상자의 로컬 축 2개씩, 총 4개다.
	// 마주보는 두 면은 방향만 반대라 축으로는 같으므로 면 8개를 다 볼 필요가 없다.
	FVector axes[4] = { a.axis[0], a.axis[1], b.axis[0], b.axis[1] };

	// a의 축들과 b의 축들에서 각각 가장 얕은 겹침을 따로 찾는다
	float overlapA = FLT_MAX;
	float overlapB = FLT_MAX;
	int indexA = 0;
	int indexB = 2;

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

		if (i < 2)
		{
			if (overlap < overlapA) { overlapA = overlap; indexA = i; }
		}
		else
		{
			if (overlap < overlapB) { overlapB = overlap; indexB = i; }
		}
	}

	// 겹침이 가장 얕은 축으로 밀어내는 게 가장 적게 움직이고 빠져나가는 길이다.
	// 다만 두 값이 비슷할 때는 a를 기준으로 붙잡아 둔다. 바닥과 블록처럼 둘 다
	// 회전이 없으면 y축 겹침이 완전히 같아서, 그냥 작은 쪽을 고르면 미세한 오차로
	// 프레임마다 기준이 뒤집힌다. 그러면 아래에서 만드는 ID도 같이 뒤집혀서
	// warm starting이 매 프레임 캐시를 놓친다.
	const float relativeTolerance = 0.95f;
	const float absoluteTolerance = 0.0001f;
	bool flip = overlapB < overlapA * relativeTolerance - absoluteTolerance;

	int minIndex = flip ? indexB : indexA;
	float minOverlap = flip ? overlapB : overlapA;

	// 축의 부호는 임의라, a가 b의 반대편으로 가도록(B -> A) 맞춰준다.
	FVector normal = axes[minIndex];
	if ((a.center - b.center).DotProduct(normal) < 0.0f)
	{
		normal = normal * -1.0f;
	}

	// 최소 침투 축을 낸 상자의 면이 '기준면'이고, 반대쪽 상자에서 그 면을
	// 가장 마주보는 면이 '상대면'이다.
	// normal은 B -> A니까, a의 면은 b를 향하고(-normal) b의 면은 a를 향한다(+normal).
	const OBB& reference = flip ? b : a;
	const OBB& incident = flip ? a : b;
	FVector referenceNormal = flip ? normal : normal * -1.0f;

	int referenceFace = BestFace(reference, referenceNormal);

	// 기준면과 가장 마주보는 면 = 법선이 가장 반대인 면
	int incidentFace = BestFace(incident, referenceNormal * -1.0f);

	// 상대면(선분)을 기준면의 양옆 평면으로 잘라낸다.
	// 기준면 밖으로 삐져나간 부분은 실제로 닿은 게 아니기 때문이다.
	FVector v0 = reference.vertex[referenceFace];
	FVector v1 = reference.vertex[(referenceFace + 1) % 4];

	FVector sideDir = v1 - v0;
	sideDir.Normalize();

	int incidentNext = (incidentFace + 1) % 4;

	// 상대면의 두 끝점은 각자 어느 꼭짓점인지로 구분한다
	ClipVertex segment[2];
	segment[0].position = incident.vertex[incidentFace];
	segment[0].id = MakeContactId(referenceFace, incidentFace, incidentFace, flip);
	segment[1].position = incident.vertex[incidentNext];
	segment[1].id = MakeContactId(referenceFace, incidentFace, incidentNext, flip);

	// v0 쪽 옆면: sideDir·x >= sideDir·v0  <=>  (-sideDir)·x <= -(sideDir·v0)
	// 여기서 잘려 생긴 점은 '꼭짓점'이 아니라 '옆면 4번에서 잘린 점'이라는 뜻의 ID를 받는다
	ClipVertex clipped[2];
	if (ClipSegment(clipped, segment, sideDir * -1.0f, -sideDir.DotProduct(v0),
		MakeContactId(referenceFace, incidentFace, 4, flip)) < 2)
	{
		return result;
	}

	// v1 쪽 옆면
	ClipVertex kept[2];
	if (ClipSegment(kept, clipped, sideDir, sideDir.DotProduct(v1),
		MakeContactId(referenceFace, incidentFace, 5, flip)) < 2)
	{
		return result;
	}

	// 남은 두 점 중 기준면에 닿은 것만 진짜 접촉이다.
	// 깊이를 점마다 따로 재는 게 핵심이다. 기울어진 블록은 한쪽 모서리가
	// 더 깊이 박혀 있고, 그 차이가 블록을 평평하게 눕히는 회전을 만든다.
	//
	// 딱 붙은(separation == 0) 기준으로 자르면 안 된다. 위치 보정이 slop 근처에서
	// 멈추기 때문에 놓인 블록은 침투가 0 언저리에서 떨리고, 그때마다 한쪽 점이
	// 사라졌다 생겼다 한다. 조금 떨어진 점까지 접촉으로 유지해서 개수를 안정시킨다.
	// 실제로 안 닿았으면 솔버가 충격량을 0으로 클램프하므로 힘은 안 생긴다.
	const float contactTolerance = 0.005f;
	float referenceOffset = referenceNormal.DotProduct(v0);

	for (int i = 0; i < 2; i++)
	{
		float separation = referenceNormal.DotProduct(kept[i].position) - referenceOffset;
		if (separation > contactTolerance)
		{
			continue;
		}

		result.points[result.pointCount].position = kept[i].position;
		result.points[result.pointCount].penetration = std::fmax(-separation, 0.0f);
		result.points[result.pointCount].id = kept[i].id;
		result.pointCount++;
	}

	if (result.pointCount == 0)
	{
		return result;
	}

	result.overlapped = true;
	result.normal = normal;

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
		return a.second.AverageContactPoint().y < b.second.AverageContactPoint().y;
		});

	for (auto& [ab, info] : abinfos)
	{
		InitContact(ab.first, ab.second, info);
	}

	// 이월받은 충격량 적용. 모든 InitContact가 끝난 뒤에 따로 돈다.
	for (auto& [ab, info] : abinfos)
	{
		WarmStartContact(ab.first, ab.second, info);
	}

	for (int i = 0; i < velocityIterations; i++)
	{
		for (auto& [ab, info] : abinfos)
		{
			// 충격량(속도) 해결
			SolveContact(ab.first, ab.second, info);
		}
	}

	// 수렴한 충격량을 다음 프레임에 넘겨준다. 매 프레임 새로 만들기 때문에
	// 이제 안 닿는 쌍이나 파괴된 물체의 항목은 자연히 사라진다.
	{
		std::unordered_map<unsigned long long, CollisionInfo> currentManifolds;
		currentManifolds.reserve(abinfos.size());

		for (auto& [ab, info] : abinfos)
		{
			currentManifolds[MakePairKey(ab.first, ab.second)] = info;
		}

		previousManifolds = std::move(currentManifolds);
	}

	// 디버그용 스냅샷. 충격량이 다 풀린 뒤라 normalImpulse가 최종값이다.
	debugContacts.clear();
	for (auto& [ab, info] : abinfos)
	{
		debugContacts.push_back(info);
	}

	for (int i = 0; i < positionIterations; i++)
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

	// 위치 보정이 끝난 뒤 얼마나 남았는지 잰다. 솔버 값을 맞추는 기준이라
	// 감지를 한 번 더 도는 값어치가 있다.
	maxPenetration = 0.0f;
	for (auto& [ab, info] : abinfos)
	{
		CollisionInfo residual = CheckCollision(ab.first, ab.second);

		for (int i = 0; i < residual.pointCount; i++)
		{
			maxPenetration = std::fmax(maxPenetration, residual.points[i].penetration);
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
		// 접촉점이 여러 개면 가장 빠르게 부딪힌 점과 충격량 합으로 판단한다
		float approachSpeed = 0.0f;
		float totalNormalImpulse = 0.0f;
		for (int i = 0; i < info.pointCount; i++)
		{
			approachSpeed = std::fmin(approachSpeed, info.points[i].initialNormalVelocity);
			totalNormalImpulse += info.points[i].normalImpulse;
		}

		if (approachSpeed > -MinDamageSpeed)          continue;   // 충분히 빠르게 부딪혔나
		if (totalNormalImpulse <= CollisionThreshold) continue;   // 충분히 셌나

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

	CollisionInfo info;
	info.normal = normal;
	info.isCollision = isCollision;
	info.pointCount = 1;
	info.points[0].position = (pointA + pointB) / 2;
	info.points[0].penetration = penetration;

	return info;
}

CollisionInfo CollisionManager::CheckCollisionRectangleRectangle(ACollider* a, ACollider* b)
{
	SATResult result = OverlapOBB(MakeOBB(a), MakeOBB(b));

	if (!result.overlapped)
	{
		return CollisionInfo();
	}

	// 매우 작은 겹침 무시 (가장 깊은 점 기준)
	float deepest = 0.0f;
	for (int i = 0; i < result.pointCount; i++)
	{
		deepest = std::fmax(deepest, result.points[i].penetration);
	}

	if (deepest <= 0.0001f)
	{
		return CollisionInfo();
	}

	CollisionInfo info;
	info.normal = result.normal;
	info.isCollision = true;
	info.pointCount = result.pointCount;

	for (int i = 0; i < result.pointCount; i++)
	{
		info.points[i].position = result.points[i].position;
		info.points[i].penetration = result.points[i].penetration;
		info.points[i].id = result.points[i].id;
	}

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

	// 원이 사각형의 어느 영역(어느 면 / 어느 모서리)에 붙어 있는지.
	// clamp에 걸린 축과 그 부호가 곧 영역이다. 원이 같은 면 위에 머무는 동안은
	// 굴러가도 이 값이 안 바뀌므로 프레임 간 추적에 쓸 수 있다.
	unsigned int regionId = 0;
	if (localX != clampedX) regionId |= (localX > 0.0f) ? 0x1u : 0x2u;
	if (localY != clampedY) regionId |= (localY > 0.0f) ? 0x4u : 0x8u;

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

	CollisionInfo info;
	info.normal = normal;
	info.isCollision = isCollision;
	info.pointCount = 1;
	info.points[0].position = (pointA + pointB) / 2;
	info.points[0].penetration = penetration;
	info.points[0].id = regionId;

	return info;
}

void CollisionManager::ResolvePosition(ACollider* a, ACollider* b, const CollisionInfo& info)
{
	FVector normal = info.normal;

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	// 스태틱 충돌 (추후 수정)
	if (invMassA + invMassB <= 0.0f)
	{
		return;
	}

	// slop과 baumgarte는 멤버라 Physics Debug에서 실시간으로 바꿀 수 있다.

	// 접촉점마다 따로 보정한다.
	// 예전엔 가장 깊은 값 하나로 물체를 통째로 평행이동했는데, 그러면 기울어진
	// 블록의 얕은 쪽 모서리가 바닥에서 들려버린다. 속도 솔버와 똑같이 r x n 항을
	// 넣어서 회전까지 보정하면, 깊은 쪽이 더 내려가면서 블록이 평평해진다.
	for (int i = 0; i < info.pointCount; i++)
	{
		const ContactPoint& point = info.points[i];

		float correctionAmount = std::fmax(point.penetration - slop, 0.0f);
		if (correctionAmount <= 0.0f)
		{
			continue;
		}

		// 앞 접촉점을 보정하면서 위치와 각도가 이미 바뀌었으므로 매번 다시 구한다
		FVector rA = point.position - a->GetLocation();
		FVector rB = point.position - b->GetLocation();

		float raxn = FVector::Cross(rA, normal);
		float rbxn = FVector::Cross(rB, normal);

		// 이 접촉점을 법선 방향으로 밀 때의 유효 질량 (회전 저항 포함)
		float validMass = invMassA + invMassB + raxn * raxn * invIA + rbxn * rbxn * invIB;
		if (validMass <= 0.0f)
		{
			continue;
		}

		float correction = baumgarte * correctionAmount / validMass;

		a->SetLocation(a->GetLocation() + normal * (correction * invMassA));
		b->SetLocation(b->GetLocation() - normal * (correction * invMassB));
		a->SetRotation(a->GetRotation() + raxn * correction * invIA);
		b->SetRotation(b->GetRotation() - rbxn * correction * invIB);
	}
}

// 충돌해결 (법선 B->A)
void CollisionManager::SolveContact(ACollider* a, ACollider* b, CollisionInfo& info)
{
	FVector normal = info.normal;

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	// 마찰 계수는 물체 쌍이 정하는 거라 접촉점마다 다르지 않다. 루프 밖에서 한 번만 구한다.
	float staticFriction = std::sqrt(a->GetStaticFriction() * b->GetStaticFriction()); // 정지 마찰 계수
	float dynamicFriction = std::sqrt(a->GetDynamicFriction() * b->GetDynamicFriction());

	for (int i = 0; i < info.pointCount; i++)
	{
		ContactPoint& point = info.points[i];

		FVector rA = point.rA;
		FVector rB = point.rB;

		FVector relativeVelocity = (a->GetVelocity() + FVector::Cross(a->GetAngularVelocity(), rA)) -
			(b->GetVelocity() + FVector::Cross(b->GetAngularVelocity(), rB)); // 상대 속도
		float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

		// 충격량 적용
		float raxn = FVector::Cross(rA, normal);
		float rbxn = FVector::Cross(rB, normal);

		float delta = -(relativeVelocityNormal - point.velocityBias) * point.normalMass;
		float oldImpulse = point.normalImpulse;
		point.normalImpulse = std::fmax(oldImpulse + delta, 0.0f);
		float applied = point.normalImpulse - oldImpulse;

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
		float deltaT = -vt * point.tangentMass;                       // 여기도 곱셈
		float oldT = point.tangentImpulse;
		float newT = oldT + deltaT;

		float maxStaticFriction = point.normalImpulse * staticFriction;

		if (std::fabs(newT) > maxStaticFriction)
		{
			// 정지 마찰 한계를 넘음 -> 미끄러짐, 운동 마찰로 클램프
			float maxDynamicFriction = point.normalImpulse * dynamicFriction;
			newT = std::clamp(newT, -maxDynamicFriction, maxDynamicFriction);
		}
		// else: 정지 마찰 범위 안 -> 그대로 적용, 즉 붙잡혀서 안 미끄러짐

		point.tangentImpulse = newT;
		float appliedT = newT - oldT;

		a->SetVelocity(a->GetVelocity() + info.tangent * (appliedT * invMassA));
		b->SetVelocity(b->GetVelocity() - info.tangent * (appliedT * invMassB));
		a->SetAngularVelocity(a->GetAngularVelocity() + raxt * appliedT * invIA);
		b->SetAngularVelocity(b->GetAngularVelocity() - rbxt * appliedT * invIB);
	}
}

void CollisionManager::InitContact(ACollider* a, ACollider* b, CollisionInfo& info)
{
	FVector normal = info.normal;

	// 접선은 법선에서 나오므로 접촉점과 무관하다. 쌍마다 한 번만 구한다.
	info.tangent = FVector::Cross(normal, 1.0f);

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	if (invMassA + invMassB <= 0.0f) return; // 스태틱 충돌

	// 지난 프레임에 이 쌍이 닿아 있었다면 그때 충격량을 이어받는다 (warm starting).
	// 0에서 다시 시작하면 반복 횟수 안에 다 못 풀어서 탑이 매 프레임 가라앉는다.
	const CollisionInfo* previous = bWarmStarting ? FindPreviousManifold(a, b) : nullptr;

	for (int i = 0; i < info.pointCount; i++)
	{
		ContactPoint& point = info.points[i];

		point.normalImpulse = 0.0f;
		point.tangentImpulse = 0.0f;
		point.normalMass = 0.0f;
		point.tangentMass = 0.0f;

		FVector rA = point.position - a->GetLocation();
		FVector rB = point.position - b->GetLocation();

		FVector relativeVelocity = (a->GetVelocity() + FVector::Cross(a->GetAngularVelocity(), rA)) -
			(b->GetVelocity() + FVector::Cross(b->GetAngularVelocity(), rB)); // 상대 속도
		float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

		const float restitutionThreshold = 1.0f; // 튜닝 값
		float restitution = (std::fabs(relativeVelocityNormal) < restitutionThreshold) ? 0.0f : 0.2f;

		// 충격량
		float raxn = FVector::Cross(rA, normal);
		float rbxn = FVector::Cross(rB, normal);
		float validMass = invMassA + invMassB + raxn * raxn * invIA + rbxn * rbxn * invIB;

		float raxt = FVector::Cross(rA, info.tangent);
		float rbxt = FVector::Cross(rB, info.tangent);
		float validMassTangent = invMassA + invMassB + raxt * raxt * invIA + rbxt * rbxt * invIB;

		point.rA = rA;
		point.rB = rB;
		point.normalMass = 1.0f / validMass;
		point.tangentMass = 1.0f / validMassTangent;
		point.initialNormalVelocity = relativeVelocityNormal;
		point.velocityBias = (relativeVelocityNormal < -restitutionThreshold) ? -restitution * relativeVelocityNormal : 0.0f;

		// 지난 프레임에 ID가 같은 접촉점이 있었으면 그 충격량에서 이어서 푼다.
		// 여기서 initialNormalVelocity는 이미 기록된 뒤라 데미지 판정은 영향을 안 받는다.
		if (previous)
		{
			for (int j = 0; j < previous->pointCount; j++)
			{
				if (previous->points[j].id != point.id)
				{
					continue;
				}

				point.normalImpulse = previous->points[j].normalImpulse;
				point.tangentImpulse = previous->points[j].tangentImpulse;
				break;
			}
		}
	}
}

// 이어받은 충격량을 실제로 물체에 적용한다.
// 모든 쌍의 InitContact가 끝난 뒤에 따로 돌려야 한다. 안 그러면 앞 쌍이 밀어낸
// 속도가 뒤 쌍의 접근 속도 측정에 섞여서 반발과 데미지 판정이 틀어진다.
void CollisionManager::WarmStartContact(ACollider* a, ACollider* b, const CollisionInfo& info)
{
	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	for (int i = 0; i < info.pointCount; i++)
	{
		const ContactPoint& point = info.points[i];

		// 법선 성분과 마찰 성분을 합친 하나의 충격량
		FVector impulse = info.normal * point.normalImpulse + info.tangent * point.tangentImpulse;

		a->SetVelocity(a->GetVelocity() + impulse * invMassA);
		b->SetVelocity(b->GetVelocity() - impulse * invMassB);
		a->SetAngularVelocity(a->GetAngularVelocity() + FVector::Cross(point.rA, impulse) * invIA);
		b->SetAngularVelocity(b->GetAngularVelocity() - FVector::Cross(point.rB, impulse) * invIB);
	}
}

unsigned long long CollisionManager::MakePairKey(const ACollider* a, const ACollider* b)
{
	return ((unsigned long long)(unsigned int)a->GetID() << 32) | (unsigned int)b->GetID();
}

const CollisionInfo* CollisionManager::FindPreviousManifold(const ACollider* a, const ACollider* b) const
{
	auto found = previousManifolds.find(MakePairKey(a, b));

	return (found != previousManifolds.end()) ? &found->second : nullptr;
}
