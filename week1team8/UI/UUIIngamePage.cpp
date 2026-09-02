#include "UUIIngamePage.h"
#include <cmath>
#include <algorithm>
#include "../Global.h"


static constexpr float TargetHUD_X = 50.0f;
static constexpr float TargetHUD_Y = 40.0f;

UUIIngamePage::UUIIngamePage()
	: UUIPage(EPageType::InGame)
{
}

UUIIngamePage::~UUIIngamePage()
{
	for (auto* ft : FloatingTexts)
	{
		delete ft;
	}
	FloatingTexts.clear();
}

bool UUIIngamePage::Initialize(IDWriteFactory* dwriteFactory, ID2D1RenderTarget* d2dRenderTarget, ID2D1Bitmap* pauseBtnBitmap, ID2D1Bitmap* bgBitmap, int screenWidth, int screenHeight)
{
	// 고정 HUD
	InGameHUD = new UUIHUDText(TargetHUD_X, TargetHUD_Y);
	if (InGameHUD->Initialize(dwriteFactory, d2dRenderTarget))
	{
		AddChild(InGameHUD);
	}

	// 일시정지 버튼
	PauseBtn = new UUIButton();
	PauseBtn->ButtonBitmap = pauseBtnBitmap;
	PauseBtn->SetPoisition(static_cast<float>(screenWidth) - 150.0f, 30.0f, 50.0f, 50.0f);
	AddChild(PauseBtn);


	return true;
}

void UUIIngamePage::AddScore(int points)
{
	TargetScore += points;
}

void UUIIngamePage::SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color)
{
	UUIFloatingText* newText = new UUIFloatingText(score, screenX, screenY, color);
	FloatingTexts.push_back(newText);
}

void UUIIngamePage::Update(float deltaTime, float mouseX, float mouseY)
{
	if (!GetVisible()) return;

	UpdateScore(deltaTime);
	UpdateFloatingTexts(deltaTime);

	UUIPage::Update(deltaTime, mouseX, mouseY);
}

void UUIIngamePage::Update(float deltaTime)
{
	Update(deltaTime, Global::MouseScreenX, Global::MouseScreenY);
}

void UUIIngamePage::UpdateScore(float deltaTime)
{
	if (DisplayScore < TargetScore)
	{
		float speed = 10.0f;
		DisplayScore += (TargetScore - DisplayScore) * (speed * deltaTime);
		if (std::abs(TargetScore - DisplayScore) < 1.0f)
		{
			DisplayScore = static_cast<float>(TargetScore);
		}
	}
}

void UUIIngamePage::UpdateFloatingTexts(float deltaTime)
{
	for (auto it = FloatingTexts.begin(); it != FloatingTexts.end(); )
	{
		UUIFloatingText* ft = *it;
		ft->Update(deltaTime);

		if (ft->bIsFinished)
		{
			if (ft->TargetScore > 0.0f)
			{
				AddScore(static_cast<int>(ft->TargetScore));
			}

			delete ft;
			it = FloatingTexts.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void UUIIngamePage::Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font)
{
	if (!GetVisible() || !renderTarget) return;

	if (InGameHUD)
	{
		InGameHUD->SetData(DisplayScore, BirdsLeft);
	}

	// Render child UI elements (HUD, Pause Button, etc.)
	UUIPage::Render(renderTarget, brush, font);

	// Render dynamic floating texts
	for (UUIFloatingText* ft : FloatingTexts)
	{
		if (ft && ft->GetVisible())
		{
			ft->Render(renderTarget, brush, font);
		}
	}
}
