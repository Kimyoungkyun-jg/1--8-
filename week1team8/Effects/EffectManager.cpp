#include "EffectManager.h"
#include <cstdlib>
#include <cmath>

bool EffectManager::Initialize()
{
	bInitialized = true;
	Clear();
	SpawnColEffect(30, L"Assets/Sprites/coleffects.png", 3, 1, 3, 0.02f);
	SpawnDisEffect(30, L"Assets/Sprites/boom.png", 5, 1, 5, 0.06f);
	return true;
}

void EffectManager::Release()
{
	Clear();
	bInitialized = false;
}

void EffectManager::Clear()
{
	for (auto* effect : colEffects)
	{
		delete effect;
	}
	colEffects.clear();

	for (auto* effect : disEffects)
	{
		delete effect;
	}
	disEffects.clear();
}

void EffectManager::SpawnColEffect(int maxCount, const wchar_t* uri, int frameX, int frameY, int totalFrames, float frameRate)
{
	for (auto* effect : colEffects)
	{
		delete effect;
	}
	colEffects.clear();

	for (int i = 0; i < maxCount; ++i)
	{
		UEffect* effect = new UEffect();
		effect->SetSpriteSheet(uri, frameX, frameY, totalFrames, frameRate, false);
		effect->Deactivate();
		colEffects.push_back(effect);
	}
}

void EffectManager::SpawnDisEffect(int maxCount, const wchar_t* uri, int frameX, int frameY, int totalFrames, float frameRate)
{
	for (auto* effect : disEffects)
	{
		delete effect;
	}
	disEffects.clear();

	for (int i = 0; i < maxCount; ++i)
	{
		UEffect* effect = new UEffect();
		effect->SetSpriteSheet(uri, frameX, frameY, totalFrames, frameRate, false);
		effect->Deactivate();
		disEffects.push_back(effect);
	}
}

UEffect* EffectManager::PlayColEffect(const FVector& worldPos, AActor* targetActor, const FVector& scale)
{
	/* colEffects 풀에서 비활성화된 이펙트 탐색 */
	for (auto* effect : colEffects)
	{
		if (effect && !effect->IsActive())
		{
			effect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale);
			if (targetActor)
			{
				effect->AttachToActor(targetActor);
			}
			return effect;
		}
	}

	/* 풀이 부족할 경우 새로 생성하여 풀에 추가 */
	UEffect* newEffect = new UEffect();
	newEffect->SetSpriteSheet(L"Assets/Sprites/coleffects.png", 3, 1, 3, 0.05f, false);
	newEffect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale);
	if (targetActor)
	{
		newEffect->AttachToActor(targetActor);
	}
	colEffects.push_back(newEffect);
	return newEffect;
}

UEffect* EffectManager::PlayDisEffect(const FVector& worldPos, const FVector& scale)
{
	/* disEffects 풀에서 비활성화된 이펙트 탐색 */
	for (auto* effect : disEffects)
	{
		if (effect && !effect->IsActive())
		{
			effect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale);
			return effect;
		}
	}

	/* 풀이 부족할 경우 새로 생성하여 풀에 추가 */
	UEffect* newEffect = new UEffect();
	newEffect->SetSpriteSheet(L"Assets/Sprites/boom.png", 5, 1, 5, 0.06f, false);
	newEffect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale);
	disEffects.push_back(newEffect);
	return newEffect;
}

void EffectManager::DeactivateAll()
{
	for (auto* effect : colEffects)
	{
		if (effect) effect->Deactivate();
	}

	for (auto* effect : disEffects)
	{
		if (effect) effect->Deactivate();
	}
}

void EffectManager::Update(float deltaTime)
{
	/* 충돌 이펙트 갱신 */
	for (auto* effect : colEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}

	/* 소멸 이펙트 갱신 */
	for (auto* effect : disEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}
}

void EffectManager::Render(URenderer& renderer)
{
	/* 충돌 이펙트 렌더링 */
	for (auto* effect : colEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}

	/* 소멸 이펙트 렌더링 */
	for (auto* effect : disEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}
}
