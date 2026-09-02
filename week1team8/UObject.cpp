#include "UObject.h"
#include "Global.h"
#include "CollisionManager.h"
#include "ObjectManager.h"
#include "TemplateLibrary.h"
#include "GameManager.h"

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

void UObject::Tick(float deltaTime)
{

}

void AActor::Draw(URenderer& renderer)
{
	renderer.UpdateConstant(Location, Rotation, Scale);
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
			BackBand->State = EBandState::Stretching;
			FrontBand->State = EBandState::Stretching;
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
	State = EBirdState::Shooting;
	bEditing = false;

	if (SlingShot)
	{
		ABand* BackBand = SlingShot->GetBackBand();
		ABand* FrontBand = SlingShot->GetFrontBand();
		if (BackBand && FrontBand)
		{
			BackBand->State = EBandState::Snapping;
			FrontBand->State = EBandState::Snapping;
			BackBand->TipLocation = Location;
			FrontBand->TipLocation = Location;
			BackBand->TipVelocity = 0;
			FrontBand->TipVelocity = 0;
		}
	}
}

void ABird::Tick(float deltaTime)
{
	if (State == EBirdState::Shooting && Velocity.LengthSquared() < 0.001f)
	{
		GameManager::GetInstance().ReloadBird();
		State = EBirdState::Shooted;
	}
}

void ASlingShot::SpawnBand()
{

	//새총의 왼쪽 위를 Back에, 오른쪽 위를 Front에
	FVector BackPoint = Location + FVector(-Scale.x / 2, Scale.y / 2, 0);
	FVector FrontPoint = Location + FVector(Scale.x / 2, Scale.y / 2, 0);
	FVector RestPoint = (BackPoint + FrontPoint) / 2;

	BackBand = SpawnActor<ABand>(BackPoint, EPrimitive::Rectangle, {0.05, 0.05, 1});
	FrontBand = SpawnActor<ABand>(FrontPoint, EPrimitive::Rectangle, { 0.05, 0.05, 1 });

	BackBand->AttachedPoint = BackPoint;
	FrontBand->AttachedPoint = FrontPoint;
	BackBand->RestPoint = RestPoint;
	FrontBand->RestPoint = RestPoint;
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
	Location = (AttachedPoint + BirdLoc) / 2;

	//밴드 길이를 밴드 포인터와 새 사이 길이만큼 증가시키고,
	// 비례해 두깨는 줄인다
	Scale.x = (AttachedPoint - BirdLoc).Length();
	Scale.y = Scaley * (min(0.5, max(1 - StretchedRate, 0.1)));

	///회전값을 구한다.
	FVector v = BirdLoc - AttachedPoint;
	Rotation = atan2f(v.y, v.x);
}

void ABand::Tick(float deltaTime)
{
	if (State == EBandState::Snapping)
	{
		TipVelocity += (RestPoint - TipLocation) * k * deltaTime;
		TipVelocity -= TipVelocity * c * deltaTime;
		TipLocation += TipVelocity * deltaTime;
		float Length = (RestPoint - TipLocation).Length();
		Stretched(TipLocation, Length / 0.6);
	}
}

float APig::minusHp()
{
	hp -= 1;
	if (hp == 0)
	{
		GameManager::GetInstance().PigDeath();
		return 0.0f;
	}

	return hp;
}
