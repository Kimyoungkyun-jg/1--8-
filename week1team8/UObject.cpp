#include "UObject.h"
#include "Global.h"
#include "enums.h"

void ACollider::Move(float t, bool bUseGravity)
{
	float deltaTime = t / 1000.0f;

	// 속도 변화
	if (bUseGravity)
	{
		Velocity += Global::G * deltaTime;
	}

	// 위치 변화
	Location += Velocity * deltaTime;

	if (Primitive == EPrimitive::Circle)
	{
		float Radius = Scale.x;
		// 공-벽 충돌 감지 및 해결
		if (Location.x < Global::leftBorder + Radius)
		{
			Velocity.x *= -1.0f;
			Location.x = Global::leftBorder + Radius;
		}
		if (Location.x > Global::rightBorder - Radius)
		{
			Velocity.x *= -1.0f;
			Location.x = Global::rightBorder - Radius;
		}
		if (Location.y < Global::bottomBorder + Radius)
		{
			Velocity.y *= -1.0f;
			Location.y = Global::bottomBorder + Radius;
		}
		if (Location.y > Global::topBorder - Radius)
		{
			Velocity.y *= -1.0f;
			Location.y = Global::topBorder - Radius;
		}
	}
}


bool ACollider::CheckCollision(UObject* Other)
{
	if (Primitive == EPrimitive::Circle)
	{
		//UBall* otherBall = dynamic_cast<UBall*>(other);
		//if (!otherBall)
		//{
		//	return false;
		//}

		//FVector diff = Location - otherBall->Location;
		//float radiusSum = (Radius + otherBall->Radius);

		//// 두 공 사이의 거리가 반지름의 합보다 작으면 충돌
		//return diff.LengthSquared() < radiusSum * radiusSum;
	}
	else if (Primitive == EPrimitive::Rectangle)
	{
		//UBall* ball = dynamic_cast<UBall*>(other);
		//if (!ball)
		//{
		//	return false;
		//}

		//float blockLeft = Location.x - Width / 2;
		//float blockRight = Location.x + Width / 2;
		//float blockUp = Location.y + Height / 2;
		//float blockDown = Location.y - Height / 2;

		//float closestX = clamp(ball->Location.x, blockLeft, blockRight);
		//float closestY = clamp(ball->Location.y, blockDown, blockUp);

		//FVector closest(closestX, closestY);
		//FVector diff = closest - ball->Location;

		//// 거리가 반지름의 합보다 작으면 충돌
		//return diff.LengthSquared() < ball->Radius * ball->Radius;
	}

	return true;
}

void ACollider::ResolveCollision(UObject* Other)
{
	if (Primitive == EPrimitive::Circle)
	{
		//UBall* otherBall = dynamic_cast<UBall*>(other);
		//if (!otherBall)
		//{
		//	return;
		//}

		//FVector diff = Location - otherBall->Location;

		//// 충돌 법선 벡터
		//FVector normal = diff;
		//normal.Normalize();

		//// 상대 속도
		//FVector v_rel = Velocity - otherBall->Velocity;

		//// 상대 속도와 법선이 이루는 각도
		//float v_rel_n = v_rel.DotProduct(normal);

		//// 멀어지고 있다면 충돌 처리를 하지 않음
		//if (v_rel_n >= 0)
		//{
		//	return;
		//}

		//// 반발 계수
		//float e = 1.0f;

		//// 충격량
		//float j = -(1.0f + e) * v_rel_n / (1.0f / Mass + 1.0f / otherBall->Mass);

		//// 속도 변화
		//Velocity += normal * (j / Mass);
		//otherBall->Velocity -= normal * (j / otherBall->Mass);

		//// 겹침 해결
		//float penetration = (Radius + otherBall->Radius) - diff.Length();

		//if (penetration < 0.01f)
		//{
		//	return;
		//}

		//FVector correction = normal * (penetration / (1.0f / Mass + 1.0f / otherBall->Mass));
		//Location += correction * (1.0f / Mass);
		//otherBall->Location -= correction * (1.0f / otherBall->Mass);
	}
	else if (Primitive == EPrimitive::Rectangle)
	{
		//UBall* ball = dynamic_cast<UBall*>(other);
		//if (!ball)
		//{
		//	return;
		//}

		//float blockLeft = Location.x - Width / 2;
		//float blockRight = Location.x + Width / 2;
		//float blockUp = Location.y + Height / 2;
		//float blockDown = Location.y - Height / 2;

		//float closestX = clamp(ball->Location.x, blockLeft, blockRight);
		//float closestY = clamp(ball->Location.y, blockDown, blockUp);

		//FVector closest(closestX, closestY);
		//FVector diff = closest - ball->Location;

		//// 충돌 법선 벡터
		//FVector normal = diff;
		//normal.Normalize();

		//// 상대 속도
		//FVector v_rel = ball->Velocity * -1.0f;

		//// 상대 속도와 법선이 이루는 각도
		//float v_rel_n = v_rel.DotProduct(normal);

		//// 멀어지고 있다면 충돌 처리를 하지 않음
		//if (v_rel_n >= 0)
		//{
		//	return;
		//}

		//// 반발 계수
		//float e = 1.0f;

		//// 충격량
		//float j = -(1.0f + e) * v_rel_n;

		//// 속도 변화
		//ball->Velocity -= normal * j;

		//// 겹침 해결
		//float penetration = ball->Radius - diff.Length();

		//if (penetration < 0.01f)
		//{
		//	return;
		//}

		//FVector correction = normal * penetration;
		//ball->Location -= correction;
	}
}


void AActor::Draw(URenderer& renderer)
{
	renderer.UpdateConstant(Location, Scale);
	renderer.RenderPrimitive(Primitive);
}
