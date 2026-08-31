#pragma once
#include <d3d11.h>
#include "Renderer.h"

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
		++TotalUObject;
	}
	virtual ~UObject()
	{
		--TotalUObject;
	}

	static int TotalUObject;
	ID3D11Buffer* VertexBuffer;
	UINT NumVertices;
};

class AActor : public UObject
{
public:
	AActor() {};
	virtual ~AActor() {};
	virtual void Draw(URenderer& renderer);				// 화면에 그리기

protected:
	FVector Location = FVector(0, 0, 0);										// 위치
	EPrimitive Primitive = EPrimitive::Circle;
	float Radius = 10.f;
};

class AColider : public AActor
{
public:
	AColider()
	{
	}
	virtual ~AColider()
	{
	}
	virtual void Move(float t, bool bUseGravity);		// t 시간동안 이동
	virtual bool CheckCollision(UObject* Other);
	virtual void ResolveCollision(UObject* Other);	// 충돌 해결 (속도 변화, 겹침 해결)

protected:
	FVector Velocity;			// 속도
	float Mass;					// 질량

};

class ABird : public AColider
{
public:
	ABird() {}
	virtual ~ABird() {}

	virtual void Draw(URenderer& renderer);
};

class AObstacle : public AColider
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