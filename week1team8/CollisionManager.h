#pragma once

#include <vector>

#include "TotalManager.h"
#include "UObject.h"

class CollisionManager : public TotalManager
{
public:
	vector<AColider*> colliders;

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

	bool IsCollide(AColider* a, AColider* b)
	{

	}

	void ResolveCollision(AColider* a, AColider* b)
	{

	}
};
