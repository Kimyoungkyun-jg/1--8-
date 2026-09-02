#include "UUIBackground.h"
#include "UIManager.h"

UUIBackground::UUIBackground()
{
	backgroundColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.6f);
}

UUIBackground::UUIBackground(float darkAlpha, float centerX, float centerY, float sizeX, float sizeY)
{
	backgroundColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, darkAlpha);
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
}

UUIBackground::UUIBackground(D2D1_COLOR_F color, float centerX, float centerY, float sizeX, float sizeY)
{
	backgroundColor = color;
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
}

UUIBackground::UUIBackground(const wchar_t* url, float centerX, float centerY, float sizeX, float sizeY)
	: imagePath(url)
{
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
	if (url)
	{
		bgBitmap = UIManager::LoadBitmapFromFile(url);
	}
}

UUIBackground::~UUIBackground()
{
	if (bgBitmap)
	{
		bgBitmap->Release();
		bgBitmap = nullptr;
	}
}

void UUIBackground::Render(ID2D1RenderTarget* renderTarget)
{
	if (!renderTarget || !GetVisible()) return;

	if (bgBitmap)
	{
		renderTarget->DrawBitmap(
			bgBitmap,
			&bgRect,
			alpha,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}
	else
	{
		ID2D1SolidColorBrush* solidBrush = nullptr;
		D2D1_COLOR_F renderColor = backgroundColor;
		renderColor.a *= alpha;
		HRESULT hr = renderTarget->CreateSolidColorBrush(renderColor, &solidBrush);
		if (SUCCEEDED(hr) && solidBrush)
		{
			renderTarget->FillRectangle(&bgRect, solidBrush);
			solidBrush->Release();
		}
	}
}