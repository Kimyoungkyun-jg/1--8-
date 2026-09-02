#pragma once

#include <vector>
#include "UEffect.h"

/* EffectManager 싱글톤 클래스 */
class EffectManager
{
public:
	/* 싱글톤 인스턴스 반환 */
	static EffectManager& GetInstance()
	{
		static EffectManager instance;
		return instance;
	}

	EffectManager(const EffectManager&) = delete;
	EffectManager& operator=(const EffectManager&) = delete;
	EffectManager(EffectManager&&) = delete;
	EffectManager& operator=(EffectManager&&) = delete;

	bool Initialize();
	void Release();

	/* 활성화된 이펙트 갱신 및 렌더링 */
	void Update(float deltaTime);
	void Render(URenderer& renderer);

	/* 이펙트 풀 사전 생성 함수 (기본 스프라이트 시트 자동 연결) */
	void SpawnColEffect(int maxCount, const wchar_t* uri = L"Assets/Sprites/coleffects.png", int frameX = 3, int frameY = 1, int totalFrames = 3, float frameRate = 0.05f);
	void SpawnDisEffect(int maxCount, const wchar_t* uri = L"Assets/Sprites/boom.png", int frameX = 5, int frameY = 1, int totalFrames = 5, float frameRate = 0.06f);
	void SpawnExpEffect(int maxCount = 5, const wchar_t* uri = L"Assets/Sprites/explosions.png", int frameX = 6, int frameY = 1, int totalFrames = 6, float frameRate = 0.06f);
	void SpawnParticleEffect(int maxCount = 100);

	UEffect* PlayColEffect(const FVector& worldPos, AActor* targetActor = nullptr, const FVector& scale = { 0.15f, 0.15f, 1.0f });
	UEffect* PlayDisEffect(const FVector& worldPos, const FVector& scale = { 0.25f, 0.25f, 1.0f });
	UEffect* PlayExpEffect(const FVector& worldPos, const FVector& scale = { 0.45f, 0.45f, 1.0f });

	void SpawnBlockDebris(const FVector& worldPos, int count = 20);

	void DeactivateAll();

	void Clear();

public:
	std::vector<UEffect*> colEffects;
	std::vector<UEffect*> disEffects;
	std::vector<UEffect*> expEffects;
	std::vector<UEffect*> particleEffects;

private:
	EffectManager() = default;
	~EffectManager() { Release(); }

	bool bInitialized = false;
};