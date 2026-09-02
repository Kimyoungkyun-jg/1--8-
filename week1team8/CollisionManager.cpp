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

	// 월드 x축, y축을 각각 angle만큼 돌린 것이 이 사각형의 로컬 축
	box.axis[0] = FVector(cs, sn, 0.0f);
	box.axis[1] = FVector(-sn, cs, 0.0f);

	box.half[0] = collider->GetScale().x * 0.5f;
	box.half[1] = collider->GetScale().y * 0.5f;

	// 중심에서 두 축 방향으로 반너비/반높이만큼 간 네 점.
	// 회전이 0일 때 좌하 -> 우하 -> 우상 -> 좌상 순서가 되고, 회전해도 순서는 유지
	FVector ex = box.axis[0] * box.half[0];
	FVector ey = box.axis[1] * box.half[1];

	box.vertex[0] = box.center - ex - ey;
	box.vertex[1] = box.center + ex - ey;
	box.vertex[2] = box.center + ex + ey;
	box.vertex[3] = box.center - ex + ey;

	// 면 i는 vertex[i] -> vertex[i+1].
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

	// 월드 좌표 <-> 물체 로컬 좌표. 물체가 움직여도 접촉점을 따라가게 하려고 쓴다.
	FVector ToLocal(const ACollider* body, const FVector& world)
	{
		FVector d = world - body->GetLocation();
		float angle = body->GetRotation();
		float cs = std::cos(angle);
		float sn = std::sin(angle);

		return FVector(d.x * cs + d.y * sn, -d.x * sn + d.y * cs, 0.0f);
	}

	FVector ToWorld(const ACollider* body, const FVector& local)
	{
		float angle = body->GetRotation();
		float cs = std::cos(angle);
		float sn = std::sin(angle);

		return body->GetLocation() + FVector(local.x * cs - local.y * sn, local.x * sn + local.y * cs, 0.0f);
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

	// 클리핑을 거치는 동안 '어디서 나온 점인지'를 위치와 함께 가짐
	struct ClipVertex
	{
		FVector position;
		unsigned int id = 0;
	};

	// 접촉점의 출처를 정수 하나로 압축
	// 기준면 / 상대면 / 어느 꼭짓점(또는 어느 옆면에서 잘렸는지) / 기준이 뒤집혔는지.
	// 이 넷이 같으면 프레임이 넘어가도 물리적으로 같은 접촉
	unsigned int MakeContactId(int referenceFace, int incidentFace, int feature, bool flip)
	{
		return (unsigned int)referenceFace
			| ((unsigned int)incidentFace << 8)
			| ((unsigned int)feature << 16)
			| ((unsigned int)(flip ? 1 : 0) << 24);
	}

	// 선분 in을 반평면 (n·x <= offset) 안쪽만 남기고 자름
	// 밖으로 나간 끝점은 평면과 만나는 지점으로 옮겨지고, 그 점은 clipId를 갖음
	int ClipSegment(ClipVertex out[2], const ClipVertex in[2], const FVector& n, float offset, unsigned int clipId)
	{
		int count = 0;

		// 평면 기준 부호 있는 거리. 음수면 안쪽.
		float d0 = n.DotProduct(in[0].position) - offset;
		float d1 = n.DotProduct(in[1].position) - offset;

		if (d0 <= 0.0f) out[count++] = in[0];
		if (d1 <= 0.0f) out[count++] = in[1];

		// 한쪽만 밖에 있으면 선분이 평면을 가로지른다. 그 교점을 추가
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
	// 사각형에서 후보가 되는 축은 각 상자의 로컬 축 2개씩, 총 4개
	FVector axes[4] = { a.axis[0], a.axis[1], b.axis[0], b.axis[1] };

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

	// 겹침이 가장 얕은 축이 법선. 두 값이 비슷하면 a를 붙잡아 둔다.
	// 안 그러면 회전이 같은 두 상자에서 미세한 오차로 기준이 매 프레임 뒤집혀
	// 아래에서 만드는 ID가 같이 흔들린다 (warm starting이 캐시를 놓침).
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

	// 최소 침투 축을 낸 상자의 면이 '기준면'.
	// normal이 B -> A라 a의 면은 b를 향하고(-normal) b의 면은 a를 향한다(+normal).
	const OBB& reference = flip ? b : a;
	const OBB& incident = flip ? a : b;
	FVector referenceNormal = flip ? normal : normal * -1.0f;

	int referenceFace = BestFace(reference, referenceNormal);

	// 기준면과 가장 마주보는 면 = 법선이 가장 반대인 면
	int incidentFace = BestFace(incident, referenceNormal * -1.0f);

	// 상대면(선분)을 기준면의 양옆 평면으로 잘라낸다.
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
	// 잘려 생긴 점은 꼭짓점이 아니므로 feature 4/5를 따로 준다
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

	// 깊이는 점마다 따로 잰다. 기울어진 블록의 좌우 깊이 차이가 블록을 눕히는 회전을 만든다.
	// separation == 0으로 자르면 놓인 블록의 침투가 slop 근처에서 떨릴 때마다 한쪽 점이
	// 사라졌다 생겼다 한다. 여유를 둬서 개수를 안정시킨다 (안 닿았으면 충격량이 0으로 클램프됨).
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

namespace
{
	// union-find. 경로를 압축하며 뿌리를 찾는다
	int FindRoot(std::vector<int>& parent, int i)
	{
		while (parent[i] != i)
		{
			parent[i] = parent[parent[i]];
			i = parent[i];
		}

		return i;
	}
}

void CollisionManager::PrimeSleep()
{
	for (ACollider* c : colliders)
	{
		if (c->GetMass() <= 0.0f)
		{
			continue;
		}

		c->SetSleepTimer(timeToSleep);
	}
}

void CollisionManager::UpdateSleep(float t, const std::vector<std::pair<ACollider*, ACollider*>>& contacts)
{
	size_t n = colliders.size();

	if (!bSleepEnabled)
	{
		for (ACollider* c : colliders)
		{
			c->WakeUp();
		}
		return;
	}

	// 1. 물체별로 '충분히 느린 상태가 이어진 시간'을 갱신한다
	float deltaTime = t / 1000.0f;

	for (ACollider* c : colliders)
	{
		if (c->GetMass() <= 0.0f)
		{
			continue;   // 정적 물체는 슬립 대상이 아니다
		}

		bool bSlow = c->GetVelocity().LengthSquared() < linearSleepTolerance * linearSleepTolerance
			&& std::fabs(c->GetAngularVelocity()) < angularSleepTolerance;

		c->SetSleepTimer(bSlow ? c->GetSleepTimer() + deltaTime : 0.0f);
	}

	// 2. 접촉으로 연결된 무리를 만든다.
	//    정적 물체는 무리를 잇지 않는다. 바닥을 통해 이으면 바닥에 닿은 모든 물체가
	//    한 덩어리가 되어, 화면 어딘가에서 하나만 움직여도 아무것도 못 잠든다.
	std::unordered_map<const ACollider*, int> indexOf;
	indexOf.reserve(n);
	for (int i = 0; i < (int)n; i++)
	{
		indexOf[colliders[i]] = i;
	}

	std::vector<int> parent(n);
	for (int i = 0; i < (int)n; i++)
	{
		parent[i] = i;
	}

	for (const auto& contact : contacts)
	{
		if (contact.first->GetMass() <= 0.0f || contact.second->GetMass() <= 0.0f)
		{
			continue;
		}

		int rootA = FindRoot(parent, indexOf[contact.first]);
		int rootB = FindRoot(parent, indexOf[contact.second]);

		if (rootA != rootB)
		{
			parent[rootA] = rootB;
		}
	}

	// 3. 무리마다 가장 짧은 타이머를 찾는다. 그게 무리 전체의 진행도다.
	std::unordered_map<int, float> islandTimer;

	for (int i = 0; i < (int)n; i++)
	{
		if (colliders[i]->GetMass() <= 0.0f)
		{
			continue;
		}

		int root = FindRoot(parent, i);
		float timer = colliders[i]->GetSleepTimer();
		auto found = islandTimer.find(root);

		if (found == islandTimer.end() || timer < found->second)
		{
			islandTimer[root] = timer;
		}
	}

	// 4. 무리 단위로 재우거나 깨운다.
	//    깨어 있는 물체가 잠든 무리에 새로 닿으면 둘이 한 무리가 되고,
	//    그 무리의 최소 타이머가 0이 되므로 잠든 쪽이 저절로 깨어난다.
	for (int i = 0; i < (int)n; i++)
	{
		ACollider* c = colliders[i];
		if (c->GetMass() <= 0.0f)
		{
			continue;
		}

		if (islandTimer[FindRoot(parent, i)] >= timeToSleep)
		{
			c->SetSleeping(true);
			c->SetVelocity(FVector());
			c->SetAngularVelocity(0.0f);
		}
		else
		{
			c->SetSleeping(false);
		}
	}
}

std::vector<CollisionInfo> CollisionManager::CheckCollisionAll(float t)
{
	std::vector<CollisionInfo> infos;
	std::vector<std::pair<std::pair<ACollider*, ACollider*>, CollisionInfo>> abinfos;
	size_t n = colliders.size();

	for (size_t i = 0; i < n; i++)
	{
		for (size_t j = i + 1; j < n; j++)
		{
			// 정적끼리는 아예 볼 것도 없다.
			// 잠든 쌍은 감지까지는 한다 — 무리를 잇고 파괴 때 깨우려면 접촉 그래프가 온전해야 한다.
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
		if (!IsPairSolvable(ab.first, ab.second)) continue;

		InitContact(ab.first, ab.second, info);
	}

	// 이월받은 충격량 적용. 모든 InitContact가 끝난 뒤에 따로 돈다.
	for (auto& [ab, info] : abinfos)
	{
		if (!IsPairSolvable(ab.first, ab.second)) continue;

		WarmStartContact(ab.first, ab.second, info);
	}

	for (int i = 0; i < velocityIterations; i++)
	{
		for (auto& [ab, info] : abinfos)
		{
			if (!IsPairSolvable(ab.first, ab.second)) continue;

			// 충격량(속도) 해결
			SolveContact(ab.first, ab.second, info);
		}
	}

	// 수렴한 충격량을 다음 프레임에 넘긴다. 매 프레임 새로 만들어 교체하므로
	// 안 닿는 쌍이나 파괴된 물체의 항목은 저절로 사라진다.
	{
		std::unordered_map<unsigned long long, CollisionInfo> currentManifolds;
		currentManifolds.reserve(abinfos.size());

		for (auto& [ab, info] : abinfos)
		{
			unsigned long long key = MakePairKey(ab.first, ab.second);

			if (IsPairSolvable(ab.first, ab.second))
			{
				currentManifolds[key] = info;
				continue;
			}

			// 잠든 쌍은 이번 프레임에 풀지 않았으므로 충격량이 0이다.
			// 그대로 저장하면 깨어날 때 warm start가 날아가고, '새 접촉'으로 잘못 잡혀
			// 데미지가 한 번 더 들어간다. 지난 값을 그대로 들고 간다.
			auto found = previousManifolds.find(key);
			if (found != previousManifolds.end())
			{
				currentManifolds[key] = found->second;
			}
		}

		previousManifolds = std::move(currentManifolds);
	}

	// 디버그용 스냅샷. 충격량이 다 풀린 뒤라 normalImpulse가 최종값이다.
	debugContacts.clear();
	for (auto& [ab, info] : abinfos)
	{
		debugContacts.push_back(info);
	}

	// 겹침 해소. ResolvePosition이 로컬 앵커로 깊이를 다시 재므로 재감지가 필요 없다.
	for (int i = 0; i < positionIterations; i++)
	{
		for (auto& [ab, info] : abinfos)
		{
			if (!IsPairSolvable(ab.first, ab.second)) continue;

			ResolvePosition(ab.first, ab.second, info);
		}
	}

	// 위치 보정 후 남은 겹침. 솔버 값 튜닝 기준이라 감지를 한 번 더 도는 값어치가 있다.
	maxPenetration = 0.0f;
	for (auto& [ab, info] : abinfos)
	{
		CollisionInfo residual = CheckCollision(ab.first, ab.second);

		for (int i = 0; i < residual.pointCount; i++)
		{
			maxPenetration = std::fmax(maxPenetration, residual.points[i].penetration);
		}
	}

	// 속도가 확정된 뒤, 물체가 파괴되기 전에 판정한다.
	// 파괴 뒤에 하면 contacts가 이미 지워진 콜라이더를 가리킨다.
	{
		std::vector<std::pair<ACollider*, ACollider*>> contacts;
		contacts.reserve(abinfos.size());
		for (auto& [ab, info] : abinfos)
		{
			contacts.push_back(ab);
		}

		UpdateSleep(t, contacts);
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
		// 한 번의 충돌이 여러 프레임 이어져도 데미지는 처음 닿은 프레임에만 준다
		if (!info.bNewContact) continue;

		// 이월받은 몫은 '위에 얹힌 무게를 떠받치는 힘'이라 충돌의 세기가 아니다.
		// 빼주지 않으면 밑에 깔린 블록일수록 세게 맞아서가 아니라 위치 때문에 먼저 깨진다.
		float approachSpeed = 0.0f;
		float impactImpulse = 0.0f;
		for (int i = 0; i < info.pointCount; i++)
		{
			approachSpeed = std::fmin(approachSpeed, info.points[i].initialNormalVelocity);
			impactImpulse += std::fmax(info.points[i].normalImpulse - info.points[i].inheritedNormalImpulse, 0.0f);
		}

		if (approachSpeed > -MinDamageSpeed)      continue;   // 충분히 빠르게 부딪혔나
		if (impactImpulse <= CollisionThreshold)  continue;   // 충분히 셌나

		infos.push_back(info);

		if (!bCanDamage) continue;

		TryKill(ab.first);
		TryKill(ab.second);
	}

	// 사라질 물체가 받치고 있던 것들을 깨운다.
	// 받침이 없어지면 접촉도 같이 사라져서, 남은 물체는 자기 혼자 무리가 된다.
	// 타이머는 이미 차 있으니 그대로 두면 공중에 뜬 채로 잠들어 있는다.
	for (auto& [ab, info] : abinfos)
	{
		for (ACollider* dying : pendingkills)
		{
			if (ab.first == dying)  ab.second->WakeUp();
			if (ab.second == dying) ab.first->WakeUp();
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

	// 원 중심을 사각형의 로컬 좌표계로. 여기선 사각형이 축 정렬이라 회전을 안 따져도 된다.
	FVector toCenter = a->GetLocation() - box.center;
	float localX = toCenter.DotProduct(box.axis[0]);
	float localY = toCenter.DotProduct(box.axis[1]);

	// 사각형 안에서 원 중심과 가장 가까운 점 (로컬 좌표)
	float clampedX = std::clamp(localX, -box.half[0], box.half[0]);
	float clampedY = std::clamp(localY, -box.half[1], box.half[1]);

	// 원이 사각형의 어느 면/모서리에 붙었는지. clamp에 걸린 축과 부호가 곧 영역이라,
	// 같은 면 위를 굴러가는 동안은 안 바뀐다 (프레임 간 추적용 ID).
	unsigned int regionId = 0;
	if (localX != clampedX) regionId |= (localX > 0.0f) ? 0x1u : 0x2u;
	if (localY != clampedY) regionId |= (localY > 0.0f) ? 0x4u : 0x8u;

	FVector normal;
	float penetration;

	if (regionId == 0)
	{
		// 어느 축도 clamp에 안 걸렸다 = 원 중심이 사각형 안.
		// 가장 가까운 점이 중심 자신이라 방향을 못 구하므로, 대신 가장 얕게
		// 빠져나갈 면을 골라 그쪽으로 밀어낸다.
		// 예전엔 여기서 충돌을 버려서, 빠른 새가 벽 중심선을 넘으면 그대로 통과했다.
		float depthX = box.half[0] - std::fabs(localX);   // 좌우 면까지 남은 거리
		float depthY = box.half[1] - std::fabs(localY);   // 위아래 면까지 남은 거리

		if (depthX < depthY)
		{
			normal = box.axis[0] * (localX < 0.0f ? -1.0f : 1.0f);
			penetration = depthX + radius;
			regionId = (localX < 0.0f) ? 0x12u : 0x11u;
		}
		else
		{
			normal = box.axis[1] * (localY < 0.0f ? -1.0f : 1.0f);
			penetration = depthY + radius;
			regionId = (localY < 0.0f) ? 0x18u : 0x14u;
		}
		// 0x10 비트를 세워서 바깥쪽 영역 ID와 겹치지 않게 한다
	}
	else
	{
		// 다시 월드 좌표로
		FVector closest = box.center + box.axis[0] * clampedX + box.axis[1] * clampedY;

		FVector diff = a->GetLocation() - closest;
		float dist = diff.Length();

		if (dist >= radius || dist <= 0.0001f)
		{
			return CollisionInfo();
		}

		normal = diff / dist;
		penetration = radius - dist;
	}

	// 충돌 지점: 원 표면의 점과 사각형 쪽 점의 중간.
	// radius - penetration은 바깥일 때 중심~사각형 거리, 안쪽일 때는 음수라
	// 빠져나갈 면 위로 투영된다.
	FVector pointOnCircle = a->GetLocation() - normal * radius;
	FVector pointOnBox = a->GetLocation() - normal * (radius - penetration);

	CollisionInfo info;
	info.normal = normal;
	info.isCollision = true;
	info.pointCount = 1;
	info.points[0].position = (pointOnCircle + pointOnBox) / 2;
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

	// 접촉점마다 따로, 회전까지 보정한다 (속도 솔버와 같은 구조).
	// 가장 깊은 값으로 통째로 평행이동하면 기울어진 블록의 얕은 쪽이 바닥에서 들린다.
	//
	// 법선은 감지 시점 값으로 고정하고, 겹친 깊이만 로컬 앵커로 다시 잰다.
	// 반복마다 재감지하면 법선이 조금씩 다른 방향으로 잡히고, 그 차이가 쌓여
	// 물체가 옆으로 밀린다. 위치 보정은 속도를 안 거치므로 마찰이 이걸 못 막는다.
	for (int i = 0; i < info.pointCount; i++)
	{
		const ContactPoint& point = info.points[i];

		// 두 앵커는 감지 시점엔 같은 점이었다. 그동안 법선 방향으로 벌어진 만큼이
		// 겹침이 줄어든 양이다.
		FVector worldA = ToWorld(a, point.localA);
		FVector worldB = ToWorld(b, point.localB);
		float penetration = point.penetration - normal.DotProduct(worldA - worldB);

		float correctionAmount = std::fmax(penetration - slop, 0.0f);
		if (correctionAmount <= 0.0f)
		{
			continue;
		}

		FVector rA = worldA - a->GetLocation();
		FVector rB = worldB - b->GetLocation();

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

	// 마찰 계수는 쌍이 정하므로 접촉점마다 같다
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

		// 구름 저항.
		// 마찰은 접촉점이 '미끄러질 때만' 작동한다. 미끄러지지 않고 구르는 공은
		// 접촉점 상대속도가 0이라 마찰 충격량이 아예 생기지 않고, 그래서 계수를
		// 아무리 올려도 안 멈춘다. 실제로 공을 멈추는 건 접촉면이 눌리며 생기는
		// 저항이므로, 상대 '각속도'를 직접 줄이는 항을 따로 둔다.
		float rollingMass = invIA + invIB;

		if (rollingMass > 0.0f)
		{
			float relativeAngular = a->GetAngularVelocity() - b->GetAngularVelocity();
			float deltaR = -relativeAngular / rollingMass;

			// 한계는 법선 하중에 비례한다. 눌리는 힘이 클수록 잘 멈춘다.
			float maxRolling = point.normalImpulse * rollingResistance;

			float oldR = point.rollingImpulse;
			point.rollingImpulse = std::clamp(oldR + deltaR, -maxRolling, maxRolling);
			float appliedR = point.rollingImpulse - oldR;

			a->SetAngularVelocity(a->GetAngularVelocity() + appliedR * invIA);
			b->SetAngularVelocity(b->GetAngularVelocity() - appliedR * invIB);
		}
	}
}

void CollisionManager::InitContact(ACollider* a, ACollider* b, CollisionInfo& info)
{
	FVector normal = info.normal;

	// 접선은 법선에서 나오므로 접촉점과 무관
	info.tangent = FVector::Cross(normal, 1.0f);

	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	if (invMassA + invMassB <= 0.0f) return; // 스태틱 충돌

	// 지난 프레임에 이 쌍이 닿아 있었는지. warm starting 여부와 무관하게 봐야
	// 데미지 판정이 "새로 부딪힌 프레임"을 제대로 가려낸다.
	const CollisionInfo* previous = FindPreviousManifold(a, b);
	info.bNewContact = (previous == nullptr);

	// warm starting: 0에서 다시 풀면 반복 안에 못 끝내서 탑이 매 프레임 가라앉는다
	if (!bWarmStarting)
	{
		previous = nullptr;
	}

	for (int i = 0; i < info.pointCount; i++)
	{
		ContactPoint& point = info.points[i];

		point.normalImpulse = 0.0f;
		point.tangentImpulse = 0.0f;
		point.normalMass = 0.0f;
		point.tangentMass = 0.0f;

		// 구름 저항은 이월하지 않는다. 방향이 바뀌면 반대로 제동을 걸어버린다.
		point.rollingImpulse = 0.0f;

		FVector rA = point.position - a->GetLocation();
		FVector rB = point.position - b->GetLocation();

		FVector relativeVelocity = (a->GetVelocity() + FVector::Cross(a->GetAngularVelocity(), rA)) -
			(b->GetVelocity() + FVector::Cross(b->GetAngularVelocity(), rB)); // 상대 속도
		float relativeVelocityNormal = normal.DotProduct(relativeVelocity); // 상대 속도의 충돌 방향 성분

		// 반발계수 조합은 둘 중 큰 쪽. 느린 충돌에서 0으로 죽이는 건 놓인 물체의 떨림 방지
		const float restitutionThreshold = 1.0f; // 튜닝 값
		float pairRestitution = std::fmax(a->GetRestitution(), b->GetRestitution());
		float restitution = (std::fabs(relativeVelocityNormal) < restitutionThreshold) ? 0.0f : pairRestitution;

		// 충격량
		float raxn = FVector::Cross(rA, normal);
		float rbxn = FVector::Cross(rB, normal);
		float validMass = invMassA + invMassB + raxn * raxn * invIA + rbxn * rbxn * invIB;

		float raxt = FVector::Cross(rA, info.tangent);
		float rbxt = FVector::Cross(rB, info.tangent);
		float validMassTangent = invMassA + invMassB + raxt * raxt * invIA + rbxt * rbxt * invIB;

		point.rA = rA;
		point.rB = rB;
		point.localA = ToLocal(a, point.position);
		point.localB = ToLocal(b, point.position);
		point.normalMass = 1.0f / validMass;
		point.tangentMass = 1.0f / validMassTangent;
		point.initialNormalVelocity = relativeVelocityNormal;
		point.velocityBias = (relativeVelocityNormal < -restitutionThreshold) ? -restitution * relativeVelocityNormal : 0.0f;

		// ID가 같은 접촉점의 지난 충격량을 이어받는다.
		// initialNormalVelocity는 위에서 이미 기록됐으므로 데미지 판정은 영향 없음
		point.inheritedNormalImpulse = 0.0f;

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
				point.inheritedNormalImpulse = point.normalImpulse;
				break;
			}
		}
	}
}

// 이어받은 충격량 적용. 모든 InitContact가 끝난 뒤에 돌려야 한다.
// 안 그러면 앞 쌍이 밀어낸 속도가 뒤 쌍의 접근 속도 측정에 섞인다.
void CollisionManager::WarmStartContact(ACollider* a, ACollider* b, const CollisionInfo& info)
{
	float invMassA = InvMass(a->GetMass());
	float invMassB = InvMass(b->GetMass());
	float invIA = InvInertia(a->GetInertia());
	float invIB = InvInertia(b->GetInertia());

	for (int i = 0; i < info.pointCount; i++)
	{
		const ContactPoint& point = info.points[i];

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
