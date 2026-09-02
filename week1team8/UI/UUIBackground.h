#pragma once

#include <d2d1.h>
#include "UUIObject.h"

class UUIBackground : public UUIObject
{
public:
	D2D1_RECT_F bgRect = D2D1::RectF(0.0f, 0.0f, 1920.0f, 1080.0f);
	ID2D1Bitmap* bgBitmap = nullptr;
	const wchar_t* imagePath = nullptr;
	float alpha = 1.0f;
	D2D1_COLOR_F backgroundColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f);
	bool bUseColor = false;

	float CenterX = 960.0f;
	float CenterY = 540.0f;
	float BaseWidth = 1920.0f;
	float BaseHeight = 1080.0f;

	// Slide-Up Animation variables
	bool bUseSlideAnimation = false;
	bool bIsSlideAnimating = false;
	float CurrentCenterY = 540.0f;
	float TargetCenterY = 540.0f;
	float SlideSpeed = 10.0f;

public:
	UUIBackground();
	UUIBackground(float darkAlpha, float centerX = 960.0f, float centerY = 540.0f, float sizeX = 1920.0f, float sizeY = 1080.0f, bool bAnimate = false, float startOffsetY = 1200.0f);
	UUIBackground(D2D1_COLOR_F color, float centerX = 960.0f, float centerY = 540.0f, float sizeX = 1920.0f, float sizeY = 1080.0f, bool bAnimate = false, float startOffsetY = 1200.0f);
	UUIBackground(const wchar_t* url, float centerX = 960.0f, float centerY = 540.0f, float sizeX = 1920.0f, float sizeY = 1080.0f, bool bAnimate = false, float startOffsetY = 1200.0f);

	virtual ~UUIBackground() override;

	void SetImagePath(const wchar_t* path)
	{
		imagePath = path;
		bUseColor = false;
	}

	void SetColor(D2D1_COLOR_F color)
	{
		backgroundColor = color;
		bUseColor = true;
	}

	void SetPoisition(float left, float top, float width, float height)
	{
		BaseWidth = width;
		BaseHeight = height;
		CenterX = left + width * 0.5f;
		CenterY = top + height * 0.5f;
		TargetCenterY = CenterY;
		CurrentCenterY = CenterY;
		bgRect = D2D1::RectF(left, top, left + width, top + height);
	}

	void SetCenterPoisition(float centerX, float centerY, float sizeX, float sizeY)
	{
		CenterX = centerX;
		CenterY = centerY;
		BaseWidth = sizeX;
		BaseHeight = sizeY;
		TargetCenterY = centerY;
		CurrentCenterY = centerY;
		float halfW = sizeX * 0.5f;
		float halfH = sizeY * 0.5f;
		bgRect = D2D1::RectF(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
	}

	void SetSlideAnimation(bool bEnable, float startOffsetY = 1200.0f, float speed = 10.0f);
	void StartSlideUp(float startY, float targetY, float speed = 10.0f);
	void ResetAnimation(float startOffsetY = 1200.0f);

	virtual void Update(float deltaTime) override;
	void Render(ID2D1RenderTarget* renderTarget);
};
