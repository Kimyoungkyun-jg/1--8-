#pragma once

#include <vector>

#include "TotalManager.h"
#include "UObject.h"

class CollisionManager : public TotalManager
{
public:
	vector<ACollider*> colliders;

	void CheckCollision()
	{
		int n = colliders.size();

		for (int i = 0; i < n; i++)
		{
			for (int j = i + 1; j < n; j++)
			{
				if (IsCollide(colliders[i], colliders[j]))
				{
					ResolveCollision(colliders[i], colliders[j]);
				}
			}
		}
	}

	bool IsCollide(ACollider* a, ACollider* b)
	{
		if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Circle)
		{

		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Rectangle)
		{

		}
		else if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Rectangle)
		{

		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Circle)
		{

		}
	}

	void ResolveCollision(ACollider* a, ACollider* b)
	{
		if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Circle)
		{

		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Rectangle)
		{

		}
		else if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Rectangle)
		{

		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Circle)
		{

		}
	}
};
