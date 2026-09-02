#pragma once

#include <vector>
#include <d2d1.h>
#include <d3d11.h>
#include "../Vector.h"
#include "../UObject.h"

/* UEffect : UObject 상속 이펙트 및 오브젝트 풀링, 대상 추적 지원 클래스 */
class UEffect : public UObject
{
public:
	UEffect();
	UEffect(const FVector& location, const FVector& velocity, float lifeTime = 1.0f, const FVector& scale = { 0.05f, 0.05f, 1.0f });
	virtual ~UEffect() override;

	virtual void Tick(float deltaTime) override;
	virtual void Draw(URenderer& renderer);

	/* 활성화 / 비활성화 제어 */
	void SetActive(bool bActive) { bIsActive = bActive; }
	bool IsActive() const { return bIsActive; }

	void Activate(const FVector& loc, const FVector& vel = { 0.0f, 0.0f, 0.0f }, const FVector& scale = { 0.05f, 0.05f, 1.0f });
	void Deactivate() { bIsActive = false; TargetActor = nullptr; }

	/* 대상 액터 추적(부착) */
	void AttachToActor(AActor* target, const FVector& offset = { 0.0f, 0.0f, 0.0f })
	{
		TargetActor = target;
		TargetOffset = offset;
	}
	void DetachFromActor() { TargetActor = nullptr; }

	void SetLocation(const FVector& loc) { Location = loc; }
	void SetVelocity(const FVector& vel) { Velocity = vel; }
	void SetScale(const FVector& scale) { Scale = scale; }
	void SetLifeTime(float life) { LifeTime = life; }
	void SetColor(D2D1_COLOR_F color) { Color = color; }

	/* 단일 이미지 설정 */
	void SetImage(const wchar_t* uri);

	/* 스프라이트 시트 설정 (가로칸수, 세로칸수, 총프레임수, 프레임당시간, 반복여부) */
	void SetSpriteSheet(const wchar_t* uri, int frameX, int frameY, int totalFrames, float frameRate = 0.05f, bool bLoop = false);

	FVector GetLocation() const { return Location; }
	FVector GetVelocity() const { return Velocity; }
	FVector GetScale() const { return Scale; }

public:
	bool bIsActive = false;                        /* 활성화 여부 */

	AActor* TargetActor = nullptr;                 /* 추적 대상 액터 */
	FVector TargetOffset = { 0.0f, 0.0f, 0.0f };   /* 추적 오프셋 */

	FVector Location = { 0.0f, 0.0f, 0.0f };
	FVector Velocity = { 0.0f, 0.0f, 0.0f };
	FVector Scale = { 0.05f, 0.05f, 1.0f };
	float Rotation = 0.0f;
	float AngularVelocity = 0.0f;

	float LifeTime = 1.0f;
	float CurrentLife = 0.0f;
	D2D1_COLOR_F Color = { 1.0f, 1.0f, 1.0f, 1.0f };

	/* 스프라이트 애니메이션 관련 변수 */
	ID2D1Bitmap* Bitmap = nullptr;
	int FrameX = 1;
	int FrameY = 1;
	int TotalFrames = 1;
	int CurrentFrame = 0;
	float FrameRate = 0.05f;
	float FrameTimer = 0.0f;
	bool bLoop = false;
};