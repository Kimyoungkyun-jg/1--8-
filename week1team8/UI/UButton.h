#pragma once

#include <d2d1.h>
#include "UUIObject.h"

class UUIButton : public UUIObject
{
public:
	UUIButton() = default;
	virtual ~UUIButton() override
	{
		if (ButtonBitmap)
		{
			ButtonBitmap->Release();
			ButtonBitmap = nullptr;
		}
	}

	void SetImagePath(const wchar_t* path)
	{
		imagePath = path;
	}

	void SetPoisition(float left, float top, float width, float height)
	{
		ButtonRect = D2D1::RectF(left, top, left+width, top+height);
	}

	void Render(ID2D1RenderTarget* renderTarget);

public:
	D2D1_RECT_F ButtonRect = D2D1::RectF(0, 0, 0, 0);
	ID2D1Bitmap* ButtonBitmap = nullptr;
	const wchar_t* imagePath = nullptr;
};
