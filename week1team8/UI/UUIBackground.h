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
	D2D1_COLOR_F backgroundColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.6f);

public:
	UUIBackground();
	UUIBackground(float darkAlpha, float centerX = 960.0f, float centerY = 540.0f, float sizeX = 1920.0f, float sizeY = 1080.0f);
	UUIBackground(D2D1_COLOR_F color, float centerX = 960.0f, float centerY = 540.0f, float sizeX = 1920.0f, float sizeY = 1080.0f);
	UUIBackground(const wchar_t* url, float centerX = 960.0f, float centerY = 540.0f, float sizeX = 1920.0f, float sizeY = 1080.0f);

	virtual ~UUIBackground() override;

	void SetImagePath(const wchar_t* path)
	{
		imagePath = path;
	}

	void SetColor(D2D1_COLOR_F color)
	{
		backgroundColor = color;
	}

	void SetPoisition(float left, float top, float width, float height)
	{
		bgRect = D2D1::RectF(left, top, left + width, top + height);
	}

	void SetCenterPoisition(float centerX, float centerY, float sizeX, float sizeY)
	{
		float halfW = sizeX * 0.5f;
		float halfH = sizeY * 0.5f;
		bgRect = D2D1::RectF(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
	}

	void Render(ID2D1RenderTarget* renderTarget);
};
