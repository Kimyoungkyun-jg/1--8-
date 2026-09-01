#pragma once
#include <d3d11.h>
#include "Renderer.h"
#include <vector>

class UObject
{
public:
	UObject()
	{
	}
	virtual ~UObject()
	{
	}
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
	void SetPrimitive(EPrimitive _Primitive) {Primitive = _Primitive;}
	float GetRotation() const { return Rotation; }
	EPrimitive GetPrimitive() const { return Primitive; }
	FVector GetScale() const { return Scale; }

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
	virtual void Move(float t, bool bUseGravity);		// t 시간동안 이동
	virtual bool CheckCollision(UObject* Other);
	virtual void ResolveCollision(UObject* Other);	// 충돌 해결 (속도 변화, 겹침 해결)
	void SetVelocity ( FVector _Vel ) { Velocity = _Vel; }
	FVector GetVelocity ( ) const { return Velocity; }

protected:
	FVector Velocity;			// 속도
	float Mass;					// 질량

};

class ABird : public ACollider
{
public:
	ABird() {}
	virtual ~ABird() {}
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
	APig() : AObstacle(1.0f) {}
	virtual ~APig() {}
};

class ABlock : public AObstacle
{
public:
	ABlock() : AObstacle(1.0f) {}
	virtual ~ABlock() {}
};