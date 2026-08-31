#pragma once
#include <d3d11.h>
#include "Renderer.h"
#include <vector>

enum class EPrimitive
{
	Circle,
	Rectangle
};

class UObject
{
public:
	UObject()
	{
	}
	virtual ~UObject()
	{
	}

	ID3D11Buffer* VertexBuffer;
	UINT NumVertices;

private:
	int index;
};

class AActor : public UObject
{
public:
	AActor() {};
	virtual ~AActor() {};
	virtual void Draw(URenderer& renderer);				// 화면에 그리기
	EPrimitive GetPrimitive()
	{
		return Primitive;
	}

	void SetLocation(const FVector& loc) { Location = loc; }
	void SetRadius ( const float _Radius ) { Radius = _Radius; }

protected:
	FVector Location = FVector(0, 0, 0);				// 위치
	EPrimitive Primitive = EPrimitive::Circle;
	float Radius = 10.f;
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
	AObstacle(float _hp = 1) : hp(_hp)
	{
	}
	virtual ~AObstacle(){}

private:
	float hp;
};

class APig : public AObstacle
{
	APig(){}
	~APig(){}
};

class ABlock : public AObstacle
{
	ABlock(){}
	~ABlock(){}
};