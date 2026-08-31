struct FVector
{
	float x, y, z;

	FVector(float _x = 0, float _y = 0, float _z = 0);

	float LengthSquared() const;
	float Length() const;
	void Normalize();
	float DotProduct(const FVector& other);

	FVector operator+(const FVector& other) const;
	FVector operator-(const FVector& other) const;
	FVector operator*(float scalar) const;
	FVector& operator+=(const FVector& other);
	FVector& operator-=(const FVector& other);
};
