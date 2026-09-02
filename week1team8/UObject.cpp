#include "UObject.h"
#include "Global.h"
#include "CollisionManager.h"
#include "ObjectManager.h"
#include "TemplateLibrary.h"

void ACollider::Move(float t)
{
	float deltaTime = t / 1000.0f;

	// 속도 변화
	if (bUseGravity)
	{
		Velocity += Global::G / 2 * deltaTime;
	}

	// 위치 변화
	Location += Velocity * deltaTime;

	if (Primitive == EPrimitive::Circle)
	{
		float Radius = Scale.x / 2;
		// 공-벽 충돌 감지 및 해결
		if (Location.x < Global::leftBorder + Radius)
		{
			Velocity.x *= -0.8f;
			Location.x = Global::leftBorder + Radius;
		}
		if (Location.x > Global::rightBorder - Radius)
		{
			Velocity.x *= -0.8f;
			Location.x = Global::rightBorder - Radius;
		}
		if (Location.y < Global::bottomBorder + Radius)
		{
			Velocity.y *= -0.8f;
			Location.y = Global::bottomBorder + Radius;
		}
		if (Location.y > Global::topBorder - Radius)
		{
			Velocity.y *= -0.8f;
			Location.y = Global::topBorder - Radius;
		}
	}
	else if (Primitive == EPrimitive::Rectangle)
	{
		float halfWidth = Scale.x * 0.5f;   // 가로 절반 
		float halfHeight = Scale.y * 0.5f;  // 세로 절반 

		// 좌/우 벽 충돌 
		if (Location.x < Global::leftBorder + halfWidth)
		{
			Velocity.x *= -0.8f;
			Location.x = Global::leftBorder + halfWidth;
		}
		if (Location.x > Global::rightBorder - halfWidth)
		{
			Velocity.x *= -0.8f;
			Location.x = Global::rightBorder - halfWidth;
		}

		// 상/하 벽 충돌
		if (Location.y < Global::bottomBorder + halfHeight)
		{
			Velocity.y *= -0.8f;
			Location.y = Global::bottomBorder + halfHeight;
		}
		if (Location.y > Global::topBorder - halfHeight)
		{
			Velocity.y *= -0.8f;
			Location.y = Global::topBorder - halfHeight;
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
	renderer.UpdateConstant(Location, Rotation, Scale);
	renderer.RenderPrimitive(Primitive);
}

void UObject::Pressed(FVector _Location)
{
	//empty
}

void UObject::Clicked()
{
	//empty
}

void UObject::Released(FVector _Location)
{
	//empty
}


void UObject::Destroy()
{
	UObjectManager::GetInstance().Destroy(this);
}

void ABird::Clicked()
{
	if (SlingShot)
	{
		ABand* BackBand = SlingShot->GetBackBand();
		ABand* FrontBand = SlingShot->GetFrontBand();
		if (BackBand && FrontBand)
		{
			BackBand->SetState(EBandState::Stretching);
			FrontBand->SetState(EBandState::Stretching);
			BackBand->Scaley = Scale.y;
			FrontBand->Scaley = Scale.y;
		}
	}
}

void ABird::Pressed(FVector _Location)
{
	Velocity = 0.f;
	bUseGravity = false;

	if (SlingShot)
	{
		ABand* BackBand = SlingShot->GetBackBand();
		ABand* FrontBand = SlingShot->GetFrontBand();
		if (BackBand && FrontBand)
		{
			//새가 이동할 수 있는 거리는 n
			//새의 위치 = AttachedPoint + 새총->새 벡터 * (n / 새총->새 벡터 길이);
			FVector Point = (BackBand->AttachedPoint + FrontBand->AttachedPoint) / 2;
			float Length = (_Location - Point).Length();
			Location = Length <= CanStretcheLength ? _Location : Point + (_Location - Point) * (CanStretcheLength / Length);

			float StretchedRate = Length / CanStretcheLength;
			BackBand->Stretched(Location, StretchedRate);
			FrontBand->Stretched(Location, StretchedRate);
		}
	}
}

void ABird::Released(FVector _Location)
{
	bUseGravity = true;

	if (SlingShot)
	{
		ABand* BackBand = SlingShot->GetBackBand();
		ABand* FrontBand = SlingShot->GetFrontBand();
		if (BackBand && FrontBand)
		{
			BackBand->SetState(EBandState::Snapping);
			FrontBand->SetState(EBandState::Snapping);
		}
	}
}

void ASlingShot::SpawnBand()
{

	//새총의 왼쪽 위를 Back에, 오른쪽 위를 Front에
	FVector BackPoint = Location + FVector(-Scale.x / 2, Scale.y / 2, 0);
	FVector FrontPoint = Location + FVector(Scale.x / 2, Scale.y / 2, 0);

	BackBand = SpawnActor<ABand>(BackPoint, EPrimitive::Rectangle, {0.05, 0.05, 1});
	FrontBand = SpawnActor<ABand>(FrontPoint, EPrimitive::Rectangle, { 0.05, 0.05, 1 });

	BackBand->AttachedPoint = BackPoint;
	FrontBand->AttachedPoint = FrontPoint;
}

void ASlingShot::Pressed(FVector _Location)
{
	if (EquippedBird)
	{
		EquippedBird->SetLocation(_Location);
		EquippedBird->SetVelocity(0.f);
		EquippedBird->bUseGravity = false;
	}
}

void ASlingShot::Released(FVector _Location)
{
	if (EquippedBird)
	{
		FVector Direction = ShotPoint - EquippedBird->GetLocation();
		EquippedBird->SetVelocity(Direction * Power);
		EquippedBird->Released(_Location);
	}
}


void AObstacle::Pressed(FVector _Location)
{
	if (bEditing)
	{
		Location = _Location;
		Velocity = 0.f;
		bUseGravity = false;
	}
}

void AObstacle::Released(FVector _Location)
{
	if (bEditing)
	{
		Location = _Location;
		Velocity = 0.f;
		bUseGravity = true;
	}
}

void ABand::Stretched(FVector BirdLoc, float StretchedRate)
{
	//밴드 포인트와 새 사이 중간점에 밴드를 위치시키고,
	Location = (AttachedPoint + BirdLoc) / 2;

	//밴드 길이를 밴드 포인터와 새 사이 길이만큼 증가시키고,
	// 비례해 두깨는 줄인다
	Scale.x = (AttachedPoint - BirdLoc).Length();
	Scale.y = Scaley * (min(0.5, max(1 - StretchedRate, 0.1)));

	//밴드 포인트와 새 사이 벡터의 tan 값으로 회전시킨다?
	FVector v = BirdLoc - AttachedPoint;
	float radian = atan2f(v.y, v.x);
	Rotation = DirectX::XMConvertToDegrees(radian);
}

void ABand::Tick()
{
}
