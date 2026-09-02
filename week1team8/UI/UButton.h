#pragma once

#include <d2d1.h>
#include "UUIObject.h"

class UUIButton : public UUIObject
{
public:
	UUIButton() = default;
	UUIButton(const wchar_t* url, float centerX, float centerY, float sizeX, float sizeY);
	virtual ~UUIButton() override;

	void SetImagePath(const wchar_t* path)
	{
		imagePath = path;
	}

	void SetPoisition(float left, float top, float width, float height)
	{
		ButtonRect = D2D1::RectF(left, top, left + width, top + height);
	}

	void SetCenterPoisition(float centerX, float centerY, float sizeX, float sizeY)
	{
		float halfW = sizeX * 0.5f;
		float halfH = sizeY * 0.5f;
		ButtonRect = D2D1::RectF(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
	}

	void Render(ID2D1RenderTarget* renderTarget);

public:
	D2D1_RECT_F ButtonRect = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
	ID2D1Bitmap* ButtonBitmap = nullptr;
	const wchar_t* imagePath = nullptr;
};
