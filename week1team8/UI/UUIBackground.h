#pragma once

#include <d2d1.h>
#include "UUIObject.h"


class UUIBackground : public UUIObject
{
public:
	UUIBackground() = default;
	virtual ~UUIBackground() override
	{
		if (bgBitmap)
		{
			bgBitmap->Release();
			bgBitmap = nullptr;
		}
	}

	void SetImagePath(const wchar_t* path)
	{
		imagePath = path;
	}

	void SetPoisition(float left, float top, float width, float height)
	{
		bgRect = D2D1::RectF(left, top, left + width, top + height);
	}

	void Render(ID2D1RenderTarget* renderTarget);

public:
	D2D1_RECT_F bgRect = D2D1::RectF(0.0f, 0.0f, 1920.0f, 1080.0f);
	ID2D1Bitmap* bgBitmap = nullptr;
	const wchar_t* imagePath = nullptr;
	float alpha = 1.0f;
};

