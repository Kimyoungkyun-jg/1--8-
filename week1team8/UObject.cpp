#include "UObject.h"
#include "Global.h"
#include "CollisionManager.h"
#include "ObjectManager.h"
#include "TemplateLibrary.h"

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

//void UObject::Destroy()
//{
//	UObjectManager::Get().Destroy(this);
//}

void AActor::Draw(URenderer& renderer)
{
	renderer.UpdateConstant(Location, Scale);
	renderer.RenderPrimitive(Primitive);
}

void ACollider::Move(float t)
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

void ACollider::Pressed(FVector _Location)
{
	if (bEditing)
	{
		Location = _Location;
		Velocity = FVector();
		bUseGravity = false;
	}
}

void ACollider::Released(FVector _Location)
{
	if (bEditing)
	{
		Location = _Location;
		Velocity = 0.f;
		bUseGravity = true;
	}
}

void ABird::Clicked()
{
	if (SlingShot)
	{
		ABand* BackBand = SlingShot->GetBackBand();
		ABand* FrontBand = SlingShot->GetFrontBand();
		if (BackBand && FrontBand)
		{
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
}

void ASlingShot::SpawnBand()
{
	BackBand = SpawnActor<ABand>(Location, EPrimitive::Rectangle);
	FrontBand = SpawnActor<ABand>(Location, EPrimitive::Rectangle);

	//새총의 왼쪽 위를 Back에, 오른쪽 위를 Front에
	FVector BackPoint = Location + FVector(-Scale.x / 2, Scale.y / 2, 0);
	FVector FrontPoint = Location + FVector(Scale.x / 2, Scale.y / 2, 0);

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

void ABand::Stretched(FVector BirdLoc, float StretchedRate)
{
	//밴드 포인트와 새 사이 중간점에 밴드를 위치시키고,
	Location = (GetLocation() + BirdLoc) / 2;

	//밴드 길이를 밴드 포인터와 새 사이 길이만큼 증가시키고,
	// 비례해 두깨는 줄인다
	Scale.x = (AttachedPoint - BirdLoc).Length();
	Scale.y = Scaley * (1 - StretchedRate);

	//밴드 포인트와 새 사이 벡터의 tan 값으로 회전시킨다?
}