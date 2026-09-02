#include "UIManager.h"
#include "../GameManager.h"
#include <cmath>


UIManager::~UIManager()
{
	Release();
}

bool UIManager::Initialize(URenderer& renderer, int nWidth, int nHeight)
{
	D2DRenderTarget = renderer.D2DRenderTarget;
	DWriteFactory = renderer.DWriteFactory;
	if (!D2DRenderTarget || !DWriteFactory) return false;

	HRESULT hr;
	hr = D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &Brush);
	if (FAILED(hr)) return false;

	hr = DWriteFactory->CreateTextFormat(
		L"맑은 고딕",
		nullptr,
		DWRITE_FONT_WEIGHT_EXTRA_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		32.0f,
		L"ko-kr",
		&FloatingFont
	);
	if (FAILED(hr)) return false;

	screenWidth = nWidth;
	screenHeight = nHeight;

	for (int i = 0; i < 4; i++)
	{
		EPageType pageType = static_cast<EPageType>(i);

		switch (pageType)
		{
		case EPageType::Starting:
		{
			UUIPage* startPage = new UUIPage(pageType);
			Pages[pageType] = startPage;
			break;
		}
		case EPageType::InGame:
		{
			UUIIngamePage* inGamePage = new UUIIngamePage();
			const wchar_t* pauseBtnPath = L"Assets/img/pausebtn.png";
			ID2D1Bitmap* pauseBtnBmp = renderer.LoadBitmapFromFile(pauseBtnPath);
			inGamePage->Initialize(DWriteFactory, D2DRenderTarget, pauseBtnBmp, nullptr, screenWidth, screenHeight);
			Pages[pageType] = inGamePage;
			break;
		}
		case EPageType::Pause:
		{
			UUIPage* pausePage = new UUIPage(pageType);
			Pages[pageType] = pausePage;
			break;
		}
		case EPageType::Ending:
		{
			UUIPage* endPage = new UUIPage(pageType);
			Pages[pageType] = endPage;
			break;
		}
		}
	}

	ChangePage(EPageType::InGame);

	return true;
}

void UIManager::AddScore(int points)
{
	if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame)))
	{
		inGame->AddScore(points);
	}
}

void UIManager::SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color)
{
	if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame)))
	{
		inGame->SpawnFloatingText(score, screenX, screenY, color);
	}
}

void UIManager::Update(float deltaTime)
{
	if (CurrentPage)
	{
		CurrentPage->Update(deltaTime);
	}
}

void UIManager::Render(int birdsLeft)
{
	if (!D2DRenderTarget)
		return;

	D2DRenderTarget->BeginDraw();

	if (CurrentPage)
	{
		if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(CurrentPage))
		{
			inGame->SetBirdsLeft(birdsLeft);
		}
		CurrentPage->Render(D2DRenderTarget, Brush, FloatingFont);
	}

	D2DRenderTarget->EndDraw();
}

void UIManager::Release()
{
	if (FloatingFont) { FloatingFont->Release(); FloatingFont = nullptr; }
	if (Brush) { Brush->Release(); Brush = nullptr; }
	D2DRenderTarget = nullptr;
	DWriteFactory = nullptr;

	for (auto& pair : Pages)
	{
		delete pair.second;
	}
	Pages.clear();
	CurrentPage = nullptr;
}

void UIManager::CalPos(EColliderId colAId, EColliderId colBId, float colposx, float colposy)
{
	float score = 0.0f;
	D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Gold);

	if ((colAId == EColliderId::BIRD && colBId == EColliderId::PIG) ||
		(colAId == EColliderId::PIG  && colBId == EColliderId::BIRD))
	{
		score += 1000.0f;
		color = D2D1::ColorF(D2D1::ColorF::Gold);
	}
	else if ((colAId == EColliderId::BIRD  && colBId == EColliderId::BLOCK) ||
		     (colAId == EColliderId::BLOCK && colBId == EColliderId::BIRD))
	{
		score = 700.0f;
		color = D2D1::ColorF(D2D1::ColorF::Yellow);
	}
	else if ((colAId == EColliderId::BLOCK && colBId == EColliderId::PIG) ||
		     (colAId == EColliderId::PIG   && colBId == EColliderId::BLOCK))
	{
		score = 2000.0f;
		color = D2D1::ColorF(D2D1::ColorF::LightGreen);
	}
	else if (colAId == EColliderId::BLOCK && colBId == EColliderId::BLOCK)
	{
		UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame));
		if (inGame)
		{
			UUIFloatingText* ft = nullptr;
			float maxMergeDistance = 100.0f;
			float minDistanceSq = maxMergeDistance * maxMergeDistance;

			for (auto& it : inGame->FloatingTexts)
			{
				if (it->bIsFlyingToHUD || it->bIsFinished) continue;
				float dx = it->X - colposx;
				float dy = it->Y - colposy;
				float distSq = dx * dx + dy * dy;

				if (distSq < minDistanceSq)
				{
					minDistanceSq = distSq;
					ft = it;
				}
			}

			if (ft)
			{
				ft->TargetScore += 500;
				ft->FloatTimer += 0.4f;
				ft->ftscale = 1.5f;
			}
		}
		return;
	}
	else
	{
		return;
	}

	SpawnFloatingText(score, colposx, colposy, color);
}

void UIManager::GetCollisionInfos(std::vector<CollisionInfo> infos) 
{
	if (infos.size() > 0)
	{
		for (auto& it : infos)
		{
			pair<float, float> pos = WorldToScreen(it.AverageContactPoint());
			CalPos(it.colAId, it.colBId, pos.first, pos.second);
		}
	}
}

std::pair<float, float> UIManager::WorldToScreen(const FVector& worldPos) 
{
	float aspect = (screenHeight > 0) ? ((float)screenWidth / (float)screenHeight) : (16.0f / 9.0f);
	float screenX = (worldPos.x / aspect + 1.0f) * 0.5f * screenWidth;
	float screenY = (1.0f - worldPos.y) * 0.5f * screenHeight;

	return { screenX, screenY };
}
