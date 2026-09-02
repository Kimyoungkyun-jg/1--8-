#pragma once
#include <d3d11.h>
#include "Renderer.h"
#include <vector>
#include "enums.h"
#include <cmath>
#include <DirectXMath.h>

class ASlingShot;

class UObject
{
public:
	UObject()
	{
		++IDMax;
		ID = IDMax;
	}
	virtual ~UObject()
	{
	}
	int GetID() const { return ID; }

	virtual void Pressed(FVector _Location);
	virtual void Clicked();
	virtual void Released(FVector _Location);
	void Destroy();

	//프레임 끝에서 Destroy 호출
	virtual void Tick(float deltaTime);

private:
	inline static int IDMax = 0;
	int ID;
};

class AActor : public UObject
{
public:
	AActor() {};
	virtual ~AActor() {};
	virtual void Draw(URenderer& renderer);

	void SetLocation(const FVector& loc) { Location = loc; }
	void SetRotation(const float _Rotation) { Rotation = _Rotation; }
	void SetScale(const FVector& _Scal) { Scale = _Scal; }
	void SetPrimitive(EPrimitive _Primitive) { Primitive = _Primitive; }
	float GetRotation() const { return Rotation; }
	EPrimitive GetPrimitive() const { return Primitive; }
	FVector GetScale() const { return Scale; }
	FVector GetLocation() const { return Location; }

	void SetImage(const wchar_t* uri)
	{
		Bitmap = URenderer::GetInstance().LoadBitmapFromFile(uri);
	}

	void SetBitmap(ID2D1Bitmap* bmp)
	{
		Bitmap = bmp;
	}

protected:
	FVector Location = FVector(0, 0, 0);				// 위치
	EPrimitive Primitive = EPrimitive::Circle;
	float Rotation = 0.0f; // 라디안, 반시계 방향
	FVector Scale = { 1, 1, 1 };

	ID2D1Bitmap* Bitmap = nullptr;

};

class ACollider : public AActor
{
public:
	ACollider() {}
	virtual ~ACollider() {}

	virtual void Move(float deltaTime);

	FVector GetVelocity() const { return Velocity; }
	void SetVelocity(FVector _Vel) { Velocity = _Vel; }

	float GetMass() const { return Mass; }
	void SetMass(float _Mass) { Mass = _Mass; }

	EColliderId GetColliderId() const { return colId; }

	float GetStaticFriction() const { return StaticFriction; }
	void SetStaticFriction(float _f) { StaticFriction = _f; }

	float GetDynamicFriction() const { return DynamicFriction; }
	void SetdynamicFriction(float _f) { DynamicFriction = _f; }

	float GetAngularVelocity() const { return AngularVelocity; }
	void SetAngularVelocity(float value) { AngularVelocity = value; }

	float GetRestitution() const { return Restitution; }
	void SetRestitution(float _r) { Restitution = _r; }

	// 슬립. 판정은 CollisionManager::UpdateSleep이 한다
	bool IsSleeping() const { return bSleeping; }
	void SetSleeping(bool value) { bSleeping = value; }
	float GetSleepTimer() const { return SleepTimer; }
	void SetSleepTimer(float value) { SleepTimer = value; }
	void WakeUp() { bSleeping = false; SleepTimer = 0.0f; }

	virtual void Pressed(FVector _Location) override;
	virtual void Released(FVector _Location) override;
	virtual float GetInertia() const = 0;

	void SetHp(float _hp) { hp = _hp; }
	virtual float minusHp() { hp -= 1; return hp; }

	bool bEditing = false;
	bool bUseGravity = true;
	bool isInvalid = false;

protected:
	EColliderId colId = EColliderId::NONE;	// collider 종류
	FVector Velocity;						// 속도
	float StaticFriction = 0.5f;
	float DynamicFriction = 0.3f;
	float Mass = 10;						// 질량
	float AngularVelocity = 0;
	float Restitution = 0.2f;	// 반발계수. 0이면 안 튀고 1이면 속도를 그대로 되돌린다
	float hp = 1.0f;
	float LinearDamping = 0.0f;
	float AngularDamping = 2.0f;
	bool bSleeping = false;
	float SleepTimer = 0.0f;	// 충분히 느린 상태가 이어진 시간
};

class ACircle : public ACollider
{
public:
	float GetRadius() const { return Scale.x / 2; }
	float GetInertia() const override
	{
		float r = GetRadius();
		return 0.5f * Mass * r * r;
	}
};

enum class EBirdState
{
	Waiting,
	Idle,
	Shooting,
	Shooted
};

class ABird : public ACircle
{
public:
	ABird()
	{
		colId = EColliderId::BIRD;
		bEditing = true;
		StaticFriction = 0.4f;   // 잘 구르고
		DynamicFriction = 0.3f;
		Restitution = 0.4f;      // 잘 튄다
	}

	virtual ~ABird() {}

	virtual void Clicked() override;
	virtual void Pressed(FVector _Location) override;
	virtual void Released(FVector _Location) override;
	virtual void Tick(float deltaTime) override;
	virtual float minusHp() override { return 1.0f; }
	virtual void Ability() {}
	void SetWait();
	void SetState(EBirdState NewState) { State = NewState; }

	EBirdState State = EBirdState::Idle;
	ASlingShot* SlingShot = nullptr;
	float CanStretcheLength = 0.6;
};

class ABombBird : public ABird
{
public:
	ABombBird(){}
	virtual ~ABombBird() {}

	virtual float minusHp();
};

class AFastBird : public ABird
{
public:
	AFastBird()
	{
		StaticFriction = 0.3f;
		DynamicFriction = 0.2f;
		Restitution = 0.5f;
	}
	virtual ~AFastBird() {}

	virtual void Ability() override;
	virtual void Clicked() override;

	bool bHasBoosted = false;
};

class APig : public ACircle
{
public:
	APig()
	{
		colId = EColliderId::PIG;
		StaticFriction = 0.5f;
		DynamicFriction = 0.4f;
		Restitution = 0.2f;
	}
	virtual ~APig() {}

	virtual float minusHp() override;
};

class ABlock : public ACollider
{
public:
	ABlock()
	{
		colId = EColliderId::BLOCK;
		StaticFriction = 0.6f;   // 나무끼리는 잘 안 미끄러지고
		DynamicFriction = 0.5f;
		Restitution = 0.05f;     // 거의 안 튄다
	}
	float GetInertia() const override
	{
		return Mass * (Scale.x * Scale.x + Scale.y * Scale.y) / 12.0f;
	}
	virtual ~ABlock() {}
};

// 화면 경계 벽. 질량 0으로 생성해서 움직이지 않는다.
class AGround : public ACollider
{
public:
	AGround()
	{
		colId = EColliderId::GROUND;
		StaticFriction = 0.7f;   // 바닥이 제일 잘 잡아준다
		DynamicFriction = 0.6f;
		Restitution = 0.1f;
	}
	float GetInertia() const override
	{
		return Mass * (Scale.x * Scale.x + Scale.y * Scale.y) / 12.0f;
	}
	virtual ~AGround() {}
};

enum class EBandState
{
	Idle,
	Stretching,
	Snapping
};

class ABand : public  AActor
{
public:
	ABand() {}
	virtual ~ABand() {}

	void Stretched(FVector BirdLoc, float StretchedRate);
	virtual void Tick(float deltaTime) override;

	float Scaley = 0.f;
	EBandState State = EBandState::Idle;
	FVector AttachedPoint;
	FVector TipLocation;
	FVector TipVelocity;
	FVector RestPoint;
	float k = 300, c = 8;
};

class ASlingShot : public AActor
{
public:
	ASlingShot() {}
	virtual ~ASlingShot() {}

	void SpawnBand();
	virtual void Pressed(FVector _Location) override;
	virtual void Released(FVector _Location) override;
	ABand* GetBackBand() { return BackBand; }
	ABand* GetFrontBand() { return FrontBand; }

	ABird* EquippedBird = nullptr;

	//새총 발사 지점
	FVector ShotPoint;

	//새총 강도
	float Power = 5.0f;
private:
	ABand* BackBand = nullptr;
	ABand* FrontBand = nullptr;
};