#include "UUIIngamePage.h"
#include "UIManager.h"
#include "../GameManager.h"
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
	TrajectoryPoints.clear();
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
	PauseBtn->SetTouch(true);
	PauseBtn->SetPoisition(static_cast<float>(screenWidth) - 130.0f, 30.0f, 100.0f, 100.0f);
	AddChild(PauseBtn);

	return true;
}

void UUIIngamePage::AddScore(int points)
{
	TargetScore += points;
}

void UUIIngamePage::ResetScore()
{
	TargetScore = 0;
	DisplayScore = 0.0f;
	BirdsLeft = GameManager::GetInstance().GetBirdCount() + (GameManager::GetInstance().GetReloadedBird() ? 1 : 0);
	if (InGameHUD)
	{
		InGameHUD->SetData(0.0f, BirdsLeft);
	}
	ClearTrajectoryPoints();
	ClearFloatingText(false);
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
	else if (DisplayScore > TargetScore)
	{
		// 점수가 리셋되었을 때 즉시 동기화
		DisplayScore = static_cast<float>(TargetScore);
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

	//포물선 궤적 점선 그리기 (새를 당기고 있을 때)
	if (!TrajectoryPoints.empty() && brush)
	{
		float baseRadius = 5.0f;
		for (size_t i = 0; i < TrajectoryPoints.size(); ++i)
		{
			const FVector& worldPt = TrajectoryPoints[i];
			std::pair<float, float> screenPt = UIManager::GetInstance().WorldToScreen(worldPt);

			float progress = static_cast<float>(i) / static_cast<float>(TrajectoryPoints.size());
			float radius = baseRadius * (1.0f - progress * 0.35f);
			float alpha = 0.85f * (1.0f - progress * 0.3f);

			brush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, alpha));
			D2D1_ELLIPSE dot = D2D1::Ellipse(
				D2D1::Point2F(screenPt.first, screenPt.second),
				radius,
				radius
			);
			renderTarget->FillEllipse(&dot, brush);
		}
	}

	if (InGameHUD)
	{
		InGameHUD->SetData(DisplayScore, BirdsLeft);
	}

	UUIPage::Render(renderTarget, brush, font);

	for (UUIFloatingText* ft : FloatingTexts)
	{
		if (ft && ft->GetVisible())
		{
			ft->Render(renderTarget, brush, font);
		}
	}
}

void UUIIngamePage::Hide()
{
	UUIPage::Hide();
	ClearTrajectoryPoints();
	ClearFloatingText(true);
}

void UUIIngamePage::ClearFloatingText(bool bAddScore)
{
	for (auto* ft : FloatingTexts)
	{
		if (ft)
		{
			if (bAddScore && ft->TargetScore > 0.0f)
			{
				AddScore(static_cast<int>(ft->TargetScore));
			}
			delete ft;
		}
	}

	FloatingTexts.clear();
	DisplayScore = static_cast<float>(TargetScore);
	if (InGameHUD)
	{
		InGameHUD->SetData(DisplayScore, BirdsLeft);
	}
}
