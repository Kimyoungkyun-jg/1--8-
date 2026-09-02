#pragma once

struct FVector
{
	float x, y, z;

	FVector(float _x = 0, float _y = 0, float _z = 0);

	float LengthSquared() const;
	float Length() const;
	void Normalize();
	float DotProduct(const FVector& other) const;

	static float Cross(FVector a, FVector b) // Z-성분
	{
		return a.x * b.y - a.y * b.x;
	}

	static FVector Cross(float w, FVector r) // w = (0, 0, w), r = 충돌지점 - 중심 거리
	{
		return FVector(-w * r.y, w * r.x, 0.0f);
	}

	static FVector Cross(FVector r, float w) // w = (0, 0, w)
	{
		return FVector(w * r.y, -w * r.x, 0.0f);
	}

	FVector operator+(const FVector& other) const;
	FVector operator-(const FVector& other) const;
	FVector operator*(float scalar) const;
	FVector operator/(float scalar) const;
	FVector& operator+=(const FVector& other);
	FVector& operator-=(const FVector& other);
};
