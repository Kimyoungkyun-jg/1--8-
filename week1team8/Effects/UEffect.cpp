#include "UEffect.h"
#include "../Global.h"

UEffect::UEffect()
{
}

UEffect::UEffect(const FVector& location, const FVector& velocity, float lifeTime, const FVector& scale)
	: Location(location), Velocity(velocity), LifeTime(lifeTime), Scale(scale), bIsActive(false)
{
}

UEffect::~UEffect()
{
	if (Bitmap)
	{
		Bitmap->Release();
		Bitmap = nullptr;
	}
}

void UEffect::Activate(const FVector& loc, const FVector& vel, const FVector& scale)
{
	Location = loc;
	Velocity = vel;
	Scale = scale;
	CurrentLife = 0.0f;
	CurrentFrame = 0;
	FrameTimer = 0.0f;
	Color.a = 1.0f;
	bIsActive = true;
	TargetActor = nullptr;
}

void UEffect::SetImage(const wchar_t* uri)
{
	if (Bitmap)
	{
		Bitmap->Release();
		Bitmap = nullptr;
	}

	FrameX = 1;
	FrameY = 1;
	TotalFrames = 1;
	CurrentFrame = 0;
	FrameTimer = 0.0f;

	if (uri)
	{
		Bitmap = URenderer::GetInstance().LoadBitmapFromFile(uri);
	}
}

void UEffect::SetSpriteSheet(const wchar_t* uri, int frameX, int frameY, int totalFrames, float frameRate, bool bLoopAnim)
{
	SetImage(uri);

	FrameX = (frameX > 0) ? frameX : 1;
	FrameY = (frameY > 0) ? frameY : 1;
	TotalFrames = (totalFrames > 0) ? totalFrames : 1;
	FrameRate = (frameRate > 0.0f) ? frameRate : 0.05f;
	bLoop = bLoopAnim;
	CurrentFrame = 0;
	FrameTimer = 0.0f;

	if (!bLoop)
	{
		LifeTime = TotalFrames * FrameRate;
	}
}

void UEffect::Tick(float deltaTime)
{
	/* 비활성화 상태에서는 업데이트 생략 */
	if (!bIsActive) return;

	/* 대상 액터가 있으면 대상 위치를 실시간으로 추적 */
	if (TargetActor)
	{
		Location = TargetActor->GetLocation() + TargetOffset;
	}
	else
	{
		/* 위치 이동 */
		Location += Velocity * deltaTime;

		/* 중력 및 감속 */
		Velocity += Global::G * 0.2f * deltaTime;
		Velocity = Velocity * 0.98f;

		/* 회전 */
		Rotation += AngularVelocity * deltaTime;
	}

	/* 스프라이트 프레임 타이머 업데이트 */
	if (TotalFrames > 1)
	{
		FrameTimer += deltaTime;
		if (FrameTimer >= FrameRate)
		{
			FrameTimer -= FrameRate;
			CurrentFrame++;

			if (CurrentFrame >= TotalFrames)
			{
				if (bLoop)
				{
					CurrentFrame = 0;
				}
				else
				{
					/* 애니메이션이 끝나면 비활성화 */
					bIsActive = false;
					TargetActor = nullptr;
					return;
				}
			}
		}
	}
	else
	{
		CurrentLife += deltaTime;
		if (CurrentLife >= LifeTime)
		{
			/* 수명이 다하면 비활성화 */
			bIsActive = false;
			TargetActor = nullptr;
			return;
		}

		/* 수명에 따른 투명도 페이드아웃 */
		float lifeRatio = 1.0f - (CurrentLife / LifeTime);
		Color.a = (lifeRatio < 0.0f) ? 0.0f : lifeRatio;
	}
}

void UEffect::Draw(URenderer& renderer)
{
	/* 비활성화 상태에서는 렌더링 생략 */
	if (!bIsActive) return;

	if (Bitmap)
	{
		if (TotalFrames > 1)
		{
			/* 스프라이트 시트 프레임 영역(srcRect) 잘라서 렌더링 */
			D2D1_SIZE_F size = Bitmap->GetSize();
			float frameW = size.width / static_cast<float>(FrameX);
			float frameH = size.height / static_cast<float>(FrameY);

			int col = CurrentFrame % FrameX;
			int row = CurrentFrame / FrameX;

			D2D1_RECT_F srcRect = D2D1::RectF(
				col * frameW,
				row * frameH,
				(col + 1) * frameW,
				(row + 1) * frameH
			);

			renderer.DrawWorldBitmap(Bitmap, Location, Rotation, Scale, Color.a, &srcRect);
		}
		else
		{
			/* 단일 이미지 전체 렌더링 */
			renderer.DrawWorldBitmap(Bitmap, Location, Rotation, Scale, Color.a);
		}
	}
	else
	{
		/* 기본 원형 프리미티브 렌더링 */
		renderer.UpdateConstant(Location, Rotation, Scale);
		renderer.RenderPrimitive(EPrimitive::Circle);
	}
}
