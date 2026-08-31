#pragma once

class FVector2
{
	float x, y;
	FVector2(float _x = 0, float _y = 0) : x(_x), y(_y) {}

	float LengthSquared() const
	{
		return x * x + y * y;
	}

	FVector2 operator+(const FVector2& other) const
	{
		return FVector2(x + other.x, y + other.y);
	}

	FVector2 operator-(const FVector2& other) const
	{
		return FVector2(x - other.x, y - other.y);
	}


	FVector2& operator+=(const FVector2& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}

	FVector2& operator-=(const FVector2& other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}
};

enum EPrimitive
{
	Circle,
	Rectangle
};

class UObject
{
public:
	UObject();
	virtual ~UObject();
};

class AActor : public UObject
{
public:
	AActor();
	virtual ~AActor();

private:
	FVector2 Transform;
	EPrimitive Primitive;
};

class AColider : public AActor
{
public:
	AColider();
	virtual ~AColider();

};
class ABird : public AColider
{
public:
	ABird();
	virtual ~ABird();
};

class AObstacle : public AColider
{
public:
	AObstacle(float _hp = 1) : hp(_hp)
	{
	};
	virtual ~AObstacle();

private:
	float hp;
};

class APig : public AObstacle
{
	APig(){}
	~APig();
};

class ABlock : public AObstacle
{
	ABlock();
	~ABlock(){}
};