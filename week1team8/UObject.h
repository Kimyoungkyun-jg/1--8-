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

protected:
	FVector Location = FVector(0, 0, 0);				// 위치
	EPrimitive Primitive = EPrimitive::Circle;
	float Rotation = 0.0f; // 라디안, 반시계 방향
	FVector Scale = { 1, 1, 1 };
};

class ACollider : public AActor
{
public:
	ACollider() {}
	virtual ~ACollider() {}

	virtual void Move(float t);		// t 시간동안 이동

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

	virtual void Pressed(FVector _Location) override;
	virtual void Released(FVector _Location) override;
	virtual float GetInertia() const = 0;

	bool bEditing = false;
	bool bUseGravity = true;

protected:
	EColliderId colId = EColliderId::NONE;	// collider 종류
	FVector Velocity;						// 속도
	float StaticFriction = 0.0f;
	float DynamicFriction = 0.0f;
	float Mass = 10;						// 질량
	float AngularVelocity = 0;
	float Restitution = 1.0f;
	float hp = 1.0f;
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

class ABird : public ACircle
{
public:
	ABird() { colId = EColliderId::BIRD; }
	virtual ~ABird() {}

	virtual void Clicked() override;
	virtual void Pressed(FVector _Location) override;
	virtual void Released(FVector _Location) override;

	ASlingShot* SlingShot = nullptr;
	float CanStretcheLength = 0.6;
};

class APig : public ACircle
{
public:
	APig() { colId = EColliderId::PIG; }
	virtual ~APig() {}
};

class ABlock : public ACollider
{
public:
	ABlock() { colId = EColliderId::BLOCK; }
	float GetInertia() const override
	{
		return Mass * (Scale.x * Scale.x + Scale.y * Scale.y) / 12.0f;
	}
	virtual ~ABlock() {}
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
	float Power = 10.f;
private:
	ABand* BackBand = nullptr;
	ABand* FrontBand = nullptr;
};