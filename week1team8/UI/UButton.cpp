#include "UButton.h"
#include "UIManager.h"

UUIButton::UUIButton(const wchar_t* url, float centerX, float centerY, float sizeX, float sizeY)
	: imagePath(url)
{
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
	if (url)
	{
		ButtonBitmap = UIManager::LoadBitmapFromFile(url);
	}
}

UUIButton::~UUIButton()
{
	if (ButtonBitmap)
	{
		ButtonBitmap->Release();
		ButtonBitmap = nullptr;
	}
}

void UUIButton::Render(ID2D1RenderTarget* renderTarget)
{
	if (ButtonBitmap && renderTarget && GetVisible())
	{
		renderTarget->DrawBitmap(
			ButtonBitmap,
			&ButtonRect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}
}
