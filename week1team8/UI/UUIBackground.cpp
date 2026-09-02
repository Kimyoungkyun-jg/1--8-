#include "UUIBackground.h"
#include "UIManager.h"

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
	if (bgBitmap && renderTarget && GetVisible())
	{
		renderTarget->DrawBitmap(
			bgBitmap,
			&bgRect,
			alpha,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}
}