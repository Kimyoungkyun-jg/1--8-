#include <cmath>

#include "Vector.h"

FVector::FVector(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

float FVector::LengthSquared() const
{
	return x * x + y * y + z * z;
}

float FVector::Length() const
{
	return sqrt(LengthSquared());
}

void FVector::Normalize()
{
	float length = Length();
	if (length > 0)
	{
		x /= length;
		y /= length;
		z /= length;
	}
}

float FVector::DotProduct(const FVector& other) const
{
	return x * other.x + y * other.y + z * other.z;
}

FVector FVector::operator+(const FVector& other) const
{
	return FVector(x + other.x, y + other.y, z + other.z);
}

FVector FVector::operator-(const FVector& other) const
{
	return FVector(x - other.x, y - other.y, z - other.z);
}

FVector FVector::operator*(float scalar) const
{
	return FVector(x * scalar, y * scalar, z * scalar);
}

FVector FVector::operator/(float scalar) const
{
	return *this * (1 / scalar);
}

FVector& FVector::operator+=(const FVector& other)
{
	x += other.x;
	y += other.y;
	z += other.z;
	return *this;
}

FVector& FVector::operator-=(const FVector& other)
{
	x -= other.x;
	y -= other.y;
	z -= other.z;
	return *this;
}
