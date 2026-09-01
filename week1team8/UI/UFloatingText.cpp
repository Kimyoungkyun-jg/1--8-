#include "UFloatingText.h"
#include <cmath>

UUIFloatingText::UUIFloatingText(float score, float x, float y, D2D1_COLOR_F color)
	: TargetScore(score)
	, CurrentScore(0.0f)
	, CustomText(L"")
	, X(x)
	, Y(y)
	, VelocityY(-45.0f)
	, FloatTimer(2.0f)
	, Color(color)
	, bIsFlyingToHUD(false)
	, bIsFinished(false)
	, ftscale(1.5f)
{
}

void UUIFloatingText::Update(float deltaTime)
{
	if (ftscale > 1.0f)
	{
		float shrinkSpeed = 6.0f;
		ftscale += (1.0f - ftscale) * (shrinkSpeed * deltaTime);
		if (ftscale < 1.001f)
		{
			ftscale = 1.0f;
		}
	}

	if (!bIsFlyingToHUD)
	{
		Y += VelocityY * deltaTime;
		FloatTimer -= deltaTime;

		if (CurrentScore < TargetScore)
		{
			float speed = 15.0f;
			CurrentScore += (TargetScore - CurrentScore) * (speed * deltaTime);
			if (std::abs(TargetScore - CurrentScore) < 1.0f)
			{
				CurrentScore = TargetScore;
			}
		}

		if (FloatTimer <= 0.0f)
		{
			CurrentScore = TargetScore;
			bIsFlyingToHUD = true;
		}
	}
	else
	{
		float dx = TargetHUD_X - X;
		float dy = TargetHUD_Y - Y;
		float dist = std::sqrt(dx * dx + dy * dy);

		if (dist < 100.0f)
		{
			bIsFinished = true;
		}
		else
		{
			float flySpeed = 2400.0f * deltaTime;
			if (flySpeed > dist) flySpeed = dist;

			X += (dx / dist) * flySpeed;
			Y += (dy / dist) * flySpeed;
		}
	}
}

void UUIFloatingText::Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font)
{
	if (!renderTarget || !brush || !font) return;

	brush->SetColor(Color);

	float width = 120.0f;
	float height = 40.0f;

	D2D1_RECT_F rect = D2D1::RectF(
		X,
		Y,
		X + width,
		Y + height
	);

	D2D1_POINT_2F center = D2D1::Point2F(X + width * 0.5f, Y + height * 0.5f);

	D2D1::Matrix3x2F scaleMatrix = D2D1::Matrix3x2F::Scale(
		D2D1::SizeF(ftscale, ftscale),
		center
	);

	renderTarget->SetTransform(scaleMatrix);

	wchar_t dynamicScoreStr[32];
	swprintf_s(dynamicScoreStr, L"%d", static_cast<int>(CurrentScore));

	renderTarget->DrawText(
		dynamicScoreStr,
		static_cast<UINT32>(wcslen(dynamicScoreStr)),
		font,
		&rect,
		brush,
		D2D1_DRAW_TEXT_OPTIONS_NONE,
		DWRITE_MEASURING_MODE_NATURAL
	);

	renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
}
