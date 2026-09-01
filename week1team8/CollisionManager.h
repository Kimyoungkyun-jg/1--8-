#pragma once

#include <vector>

#include "TotalManager.h"
#include "UObject.h"

class CollisionManager : public TotalManager
{
public:
	std::vector<ACollider*> colliders;

	void CheckCollisionAll()
	{
		int n = colliders.size();

		for (int i = 0; i < n; i++)
		{
			for (int j = i + 1; j < n; j++)
			{
				CheckCollision(colliders[i], colliders[j]);
			}
		}
	}

	// 충돌 감지 및 해결
	void CheckCollision(AColider* a, AColider* b)
	{
		if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Circle)
		{
			CheckCollisionCircleCircle(a, b);
		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Rectangle)
		{
			CheckCollisionRectangleRectangle(a, b);
		}
		else if (a->GetPrimitive() == EPrimitive::Circle && b->GetPrimitive() == EPrimitive::Rectangle)
		{
			CheckCollisionCircleRectangle(a, b);
		}
		else if (a->GetPrimitive() == EPrimitive::Rectangle && b->GetPrimitive() == EPrimitive::Circle)
		{
			CheckCollisionCircleRectangle(b, a);
		}
	}

	// 위치, 크기(가로, 세로, 반지름), 회전, 속도, 무게
	void CheckCollisionCircleCircle(AColider* a, AColider* b)
	{

	}

	void CheckCollisionRectangleRectangle(AColider* a, AColider* b)
	{

	}

	void CheckCollisionCircleRectangle(AColider* a, AColider* b)
	{

	}
};
