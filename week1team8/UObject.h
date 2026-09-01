#pragma once
#include <d3d11.h>
#include "Renderer.h"
#include <vector>
#include "enums.h"


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
	float Rotation;
	FVector Scale = { 1, 1, 1 };
};

class ACollider : public AActor
{
public:
	ACollider()
	{

	}
	virtual ~ACollider()
	{
	}
	virtual void Move(float t);		// t 시간동안 이동
	virtual bool CheckCollision(UObject* Other);
	virtual void ResolveCollision(UObject* Other);	// 충돌 해결 (속도 변화, 겹침 해결)
	void SetVelocity(FVector _Vel) { Velocity = _Vel; }
	void SetMass(float _Mass) { Mass = _Mass; }
	FVector GetVelocity() const { return Velocity; }
	float GetMass() const { return Mass; }
	EColliderId GetColliderId() const { return colId; }

	bool bUseGravity = true;

protected:
	FVector Velocity;						// 속도
	float Mass = 10;						// 질량
	EColliderId colId = EColliderId::NONE;	// collider 종류
};

class ABird : public ACollider
{
public:
	ABird() { colId = EColliderId::BIRD; }
	virtual ~ABird() {}

	virtual void Pressed(FVector _Location) override;
	virtual void Released(FVector _Location) override;
};

class AObstacle : public ACollider
{
public:
	AObstacle(float _hp = 1.0f) : hp(_hp) {}
	virtual ~AObstacle() {}

protected:
	float hp = 1.0f;
};

class APig : public AObstacle
{
public:
	APig() : AObstacle(1.0f) { colId = EColliderId::PIG; }
	virtual ~APig() {}
};

class ABlock : public AObstacle
{
public:
	ABlock() : AObstacle(1.0f) { colId = EColliderId::BLOCK; }
	virtual ~ABlock() {}
};

class ASlingShot : public AActor
{
public:
	ASlingShot() { }
	virtual ~ASlingShot() {}

	virtual void Pressed(FVector _Location) override;
	virtual void Released(FVector _Location) override;

	ABird* EquippedBird;

	//새총 발사 지점
	FVector ShotPoint;

	//새총 강도
	float Power = 7.f;
};