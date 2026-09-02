#include "UUIBackground.h"
#include "UIManager.h"
#include <cmath>

UUIBackground::UUIBackground()
{
	bUseColor = false;
}

UUIBackground::UUIBackground(float darkAlpha, float centerX, float centerY, float sizeX, float sizeY, bool bAnimate, float startOffsetY)
{
	backgroundColor = D2D1::ColorF(0.0f, 0.0f, 0.0f, darkAlpha);
	bUseColor = true;
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
	if (bAnimate)
	{
		SetSlideAnimation(true, startOffsetY);
	}
}

UUIBackground::UUIBackground(D2D1_COLOR_F color, float centerX, float centerY, float sizeX, float sizeY, bool bAnimate, float startOffsetY)
{
	backgroundColor = color;
	bUseColor = true;
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
	if (bAnimate)
	{
		SetSlideAnimation(true, startOffsetY);
	}
}

UUIBackground::UUIBackground(const wchar_t* url, float centerX, float centerY, float sizeX, float sizeY, bool bAnimate, float startOffsetY)
	: imagePath(url)
{
	bUseColor = false;
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
	if (url)
	{
		bgBitmap = UIManager::LoadBitmapFromFile(url);
	}
	if (bAnimate)
	{
		SetSlideAnimation(true, startOffsetY);
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

void UUIBackground::SetSlideAnimation(bool bEnable, float startOffsetY, float speed)
{
	bUseSlideAnimation = bEnable;
	SlideSpeed = speed;
	if (bEnable)
	{
		CurrentCenterY = startOffsetY;
		bIsSlideAnimating = true;

		float halfW = BaseWidth * 0.5f;
		float halfH = BaseHeight * 0.5f;
		bgRect = D2D1::RectF(
			CenterX - halfW,
			CurrentCenterY - halfH,
			CenterX + halfW,
			CurrentCenterY + halfH
		);
	}
}

void UUIBackground::StartSlideUp(float startY, float targetY, float speed)
{
	bUseSlideAnimation = true;
	SlideSpeed = speed;
	TargetCenterY = targetY;
	CurrentCenterY = startY;
	bIsSlideAnimating = true;
}

void UUIBackground::ResetAnimation(float startOffsetY)
{
	if (bUseSlideAnimation)
	{
		CurrentCenterY = startOffsetY;
		bIsSlideAnimating = true;

		float halfW = BaseWidth * 0.5f;
		float halfH = BaseHeight * 0.5f;
		bgRect = D2D1::RectF(
			CenterX - halfW,
			CurrentCenterY - halfH,
			CenterX + halfW,
			CurrentCenterY + halfH
		);
	}
}

void UUIBackground::Update(float deltaTime)
{
	if (!GetVisible()) return;

	if (bIsSlideAnimating)
	{
		CurrentCenterY += (TargetCenterY - CurrentCenterY) * (SlideSpeed * deltaTime);
		if (std::abs(TargetCenterY - CurrentCenterY) < 0.5f)
		{
			CurrentCenterY = TargetCenterY;
			bIsSlideAnimating = false;
		}

		float halfW = BaseWidth * 0.5f;
		float halfH = BaseHeight * 0.5f;
		bgRect = D2D1::RectF(
			CenterX - halfW,
			CurrentCenterY - halfH,
			CenterX + halfW,
			CurrentCenterY + halfH
		);
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
	else if (bUseColor)
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