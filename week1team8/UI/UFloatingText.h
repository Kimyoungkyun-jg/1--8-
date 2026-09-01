#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include <string>
#include "UUIObject.h"

class UUIFloatingText : public UUIObject
{
public:
	float TargetScore;
	float CurrentScore;
	std::wstring CustomText;

	float X;
	float Y;
	float VelocityY;
	float FloatTimer;
	D2D1_COLOR_F Color;

	float ftscale = 1.0f;

	bool bIsFlyingToHUD;
	bool bIsFinished;

	static constexpr float TargetHUD_X = 50.0f;
	static constexpr float TargetHUD_Y = 40.0f;

public:
	UUIFloatingText(float score, float x, float y, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f });

	void Update(float deltaTime);
	void Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font);
};
