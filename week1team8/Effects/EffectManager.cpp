#include "EffectManager.h"
#include "../UObject.h"
#include <cstdlib>
#include <cmath>

bool EffectManager::Initialize()
{
	bInitialized = true;
	Clear();
	SpawnColEffect(30, L"Assets/Sprites/coleffects.png", 3, 1, 3, 0.02f);
	SpawnDisEffect(30, L"Assets/Sprites/boom.png", 5, 1, 5, 0.09f);
	SpawnExpEffect(5, L"Assets/Sprites/explosions.png", 6, 1, 6, 0.06f);
	SpawnParticleEffect(100);
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

	for (auto* effect : expEffects)
	{
		delete effect;
	}
	expEffects.clear();

	for (auto* effect : blockptrs)
	{
		delete effect;
	}
	blockptrs.clear();

	for (auto* effect : iceptrs)
	{
		delete effect;
	}
	iceptrs.clear();

	for (auto* effect : rockptrs)
	{
		delete effect;
	}
	rockptrs.clear();
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

void EffectManager::SpawnExpEffect(int maxCount, const wchar_t* uri, int frameX, int frameY, int totalFrames, float frameRate)
{
	for (auto* effect : expEffects)
	{
		delete effect;
	}
	expEffects.clear();

	for (int i = 0; i < maxCount; ++i)
	{
		UEffect* effect = new UEffect();
		effect->SetSpriteSheet(uri, frameX, frameY, totalFrames, frameRate, false);
		effect->Deactivate();
		expEffects.push_back(effect);
	}
}

void EffectManager::SpawnParticleEffect(int maxCount)
{
	for (auto* effect : blockptrs) { delete effect; }
	blockptrs.clear();
	for (auto* effect : iceptrs) { delete effect; }
	iceptrs.clear();
	for (auto* effect : rockptrs) { delete effect; }
	rockptrs.clear();

	ID2D1Bitmap* blockBitmaps[3] = {
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/blockptc1.png"),
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/blockptc2.png"),
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/blockptc3.png")
	};

	ID2D1Bitmap* iceBitmaps[3] = {
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/iceptc1.png"),
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/iceptc2.png"),
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/iceptc3.png")
	};

	ID2D1Bitmap* rockBitmaps[2] = {
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/Rock1.png"),
		URenderer::GetInstance().LoadBitmapFromFile(L"Assets/img/rock2.png")
	};

	for (int i = 0; i < maxCount; ++i)
	{
		UEffect* effect = new UEffect();
		if (blockBitmaps[i % 3]) effect->SetBitmap(blockBitmaps[i % 3]);
		else effect->SetImage(L"Assets/img/blockptc1.png");
		effect->Deactivate();
		blockptrs.push_back(effect);
	}

	for (int i = 0; i < maxCount; ++i)
	{
		UEffect* effect = new UEffect();
		if (iceBitmaps[i % 3]) effect->SetBitmap(iceBitmaps[i % 3]);
		else effect->SetImage(L"Assets/img/iceptc1.png");
		effect->Deactivate();
		iceptrs.push_back(effect);
	}

	for (int i = 0; i < maxCount; ++i)
	{
		UEffect* effect = new UEffect();
		if (rockBitmaps[i % 2]) effect->SetBitmap(rockBitmaps[i % 2]);
		else effect->SetImage(L"Assets/img/Rock1.png");
		effect->Deactivate();
		rockptrs.push_back(effect);
	}
}

void EffectManager::SpawnBlockDebris(const FVector& worldPos, int count, EBlockType blockType)
{
	if (blockType == EBlockType::Ice1 || blockType == EBlockType::Ice2)
	{
		SpawnIceDebris(worldPos, count);
		return;
	}
	else if (blockType == EBlockType::Rock1 || blockType == EBlockType::Rock2)
	{
		SpawnRockDebris(worldPos, count);
		return;
	}

	const wchar_t* ptcFiles[3] = {
		L"Assets/img/blockptc1.png",
		L"Assets/img/blockptc2.png",
		L"Assets/img/blockptc3.png"
	};

	for (int i = 0; i < count; ++i)
	{
		UEffect* p = nullptr;
		for (auto* effect : blockptrs)
		{
			if (effect && !effect->IsActive())
			{
				p = effect;
				break;
			}
		}

		if (!p)
		{
			p = new UEffect();
			p->SetImage(ptcFiles[rand() % 3]);
			blockptrs.push_back(p);
		}

		float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.2831853f;
		float speed = 0.8f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.2f;

		FVector vel(cosf(angle) * speed, sinf(angle) * speed + 0.6f, 0.0f);
		float lifeTime = 0.35f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.35f;
		float size = 0.025f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.025f;

		p->Activate(worldPos, vel, FVector(size, size, 1.0f));
		p->SetLifeTime(lifeTime);
		p->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
		p->Rotation = angle;
		p->AngularVelocity = ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f) * 14.0f;
	}
}

void EffectManager::SpawnIceDebris(const FVector& worldPos, int count)
{
	const wchar_t* ptcFiles[3] = {
		L"Assets/img/iceptc1.png",
		L"Assets/img/iceptc2.png",
		L"Assets/img/iceptc3.png"
	};

	for (int i = 0; i < count; ++i)
	{
		UEffect* p = nullptr;
		for (auto* effect : iceptrs)
		{
			if (effect && !effect->IsActive())
			{
				p = effect;
				break;
			}
		}

		if (!p)
		{
			p = new UEffect();
			p->SetImage(ptcFiles[rand() % 3]);
			iceptrs.push_back(p);
		}

		float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.2831853f;
		float speed = 0.8f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.2f;

		FVector vel(cosf(angle) * speed, sinf(angle) * speed + 0.6f, 0.0f);
		float lifeTime = 0.35f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.35f;
		float size = 0.025f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.025f;

		p->Activate(worldPos, vel, FVector(size, size, 1.0f));
		p->SetLifeTime(lifeTime);
		p->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
		p->Rotation = angle;
		p->AngularVelocity = ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f) * 14.0f;
	}
}

void EffectManager::SpawnRockDebris(const FVector& worldPos, int count)
{
	const wchar_t* ptcFiles[2] = {
		L"Assets/img/Rock1.png",
		L"Assets/img/rock2.png"
	};

	for (int i = 0; i < count; ++i)
	{
		UEffect* p = nullptr;
		for (auto* effect : rockptrs)
		{
			if (effect && !effect->IsActive())
			{
				p = effect;
				break;
			}
		}

		if (!p)
		{
			p = new UEffect();
			p->SetImage(ptcFiles[rand() % 2]);
			rockptrs.push_back(p);
		}

		float angle = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 6.2831853f;
		float speed = 0.8f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.2f;

		FVector vel(cosf(angle) * speed, sinf(angle) * speed + 0.6f, 0.0f);
		float lifeTime = 0.35f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.35f;
		float size = 0.02f + (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 0.02f;

		p->Activate(worldPos, vel, FVector(size, size, 1.0f));
		p->SetLifeTime(lifeTime);
		p->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f));
		p->Rotation = angle;
		p->AngularVelocity = ((static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) - 0.5f) * 14.0f;
	}
}

UEffect* EffectManager::PlayColEffect(const FVector& worldPos, AActor* targetActor, const FVector& scale)
{
	/* colEffects 풀에서 비활성화된 이펙트 탐색 */
	for (auto* effect : colEffects)
	{
		if (effect && !effect->IsActive())
		{
			effect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale* 1.5f);
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
	newEffect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale * 2.0f);
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

UEffect* EffectManager::PlayExpEffect(const FVector& worldPos, const FVector& scale)
{
	for (auto* effect : expEffects)
	{
		if (effect && !effect->IsActive())
		{
			effect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale);
			return effect;
		}
	}
	
	/* 풀이 부족할 경우 새로 생성하여 풀에 추가 */
	UEffect* newEffect = new UEffect();
	newEffect->SetSpriteSheet(L"Assets/Sprites/explosions.png", 6, 1, 6, 0.06f, false);
	newEffect->Activate(worldPos, FVector(0.0f, 0.0f, 0.0f), scale);
	expEffects.push_back(newEffect);
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

	for (auto* effect : expEffects)
	{
		if (effect) effect->Deactivate();
	}

	for (auto* effect : blockptrs)
	{
		if (effect) effect->Deactivate();
	}

	for (auto* effect : iceptrs)
	{
		if (effect) effect->Deactivate();
	}

	for (auto* effect : rockptrs)
	{
		if (effect) effect->Deactivate();
	}
}

void EffectManager::Update(float deltaTime)
{
	for (auto* effect : colEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}

	for (auto* effect : disEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}

	for (auto* effect : expEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}

	for (auto* effect : blockptrs)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}

	for (auto* effect : iceptrs)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}

	for (auto* effect : rockptrs)
	{
		if (effect && effect->IsActive())
		{
			effect->Tick(deltaTime);
		}
	}
}

void EffectManager::Render(URenderer& renderer)
{
	for (auto* effect : colEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}

	for (auto* effect : disEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}

	for (auto* effect : expEffects)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}

	for (auto* effect : blockptrs)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}

	for (auto* effect : iceptrs)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}

	for (auto* effect : rockptrs)
	{
		if (effect && effect->IsActive())
		{
			effect->Draw(renderer);
		}
	}
}
