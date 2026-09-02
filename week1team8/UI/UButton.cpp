#include "UButton.h"
#include "UIManager.h"
#include "../Global.h"
#include <cmath>

UUIButton::UUIButton(const wchar_t* url, float centerX, float centerY, float sizeX, float sizeY, bool bAnimate, float startOffsetY)
	: imagePath(url)
{
	SetTouch(true);
	SetCenterPoisition(centerX, centerY, sizeX, sizeY);
	if (url)
	{
		ButtonBitmap = UIManager::LoadBitmapFromFile(url);
	}
	if (bAnimate)
	{
		SetSlideAnimation(true, startOffsetY);
	}
}

UUIButton::~UUIButton()
{
	if (ButtonBitmap)
	{
		ButtonBitmap->Release();
		ButtonBitmap = nullptr;
	}
	if (TextFormat)
	{
		TextFormat->Release();
		TextFormat = nullptr;
	}
}

void UUIButton::SetText(const std::wstring& text, float offsetX, float offsetY, D2D1_COLOR_F color, float fontSize)
{
	Text = text;
	TextOffsetX = offsetX;
	TextOffsetY = offsetY;
	TextColor = color;

	if (TextFormat)
	{
		TextFormat->Release();
		TextFormat = nullptr;
	}

	IDWriteFactory* dwriteFactory = URenderer::GetInstance().DWriteFactory;
	if (dwriteFactory)
	{
		dwriteFactory->CreateTextFormat(
			L"Malgun Gothic",
			nullptr,
			DWRITE_FONT_WEIGHT_BOLD,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			fontSize,
			L"ko-kr",
			&TextFormat
		);
		if (TextFormat)
		{
			TextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			TextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		}
	}
}

void UUIButton::SetSlideAnimation(bool bEnable, float startOffsetY, float speed)
{
	bUseSlideAnimation = bEnable;
	SlideSpeed = speed;
	if (bEnable)
	{
		CurrentCenterY = startOffsetY;
		bIsSlideAnimating = true;
	}
}

void UUIButton::StartSlideUp(float startY, float targetY, float speed)
{
	bUseSlideAnimation = true;
	SlideSpeed = speed;
	TargetCenterY = targetY;
	CurrentCenterY = startY;
	bIsSlideAnimating = true;
}

void UUIButton::ResetAnimation(float startOffsetY)
{
	if (bUseSlideAnimation)
	{
		CurrentCenterY = startOffsetY;
		bIsSlideAnimating = true;
	}
}

bool UUIButton::IsPointInside(float x, float y) const
{
	float halfW = (BaseWidth * 0.5f) * CurrentScale;
	float halfH = (BaseHeight * 0.5f) * CurrentScale;
	float curY = bIsSlideAnimating ? CurrentCenterY : CenterY;

	return (x >= CenterX - halfW && x <= CenterX + halfW &&
	        y >= curY - halfH && y <= curY + halfH);
}

void UUIButton::OnMouseMove(float mouseX, float mouseY)
{
	if (!GetVisible() || !GetTouch()) return;
	bIsHovered = IsPointInside(mouseX, mouseY);
}

bool UUIButton::OnMouseDown(float mouseX, float mouseY)
{
	if (!GetVisible() || !GetTouch()) return false;
	if (IsPointInside(mouseX, mouseY))
	{
		bIsPressed = true;
		return true;
	}
	return false;
}

void UUIButton::OnMouseUp(float mouseX, float mouseY)
{
	if (!GetVisible() || !GetTouch()) return;
	if (bIsPressed && IsPointInside(mouseX, mouseY))
	{
		if (OnClick)
		{
			OnClick();
		}
	}
	bIsPressed = false;
}

void UUIButton::Update(float deltaTime, float mouseX, float mouseY)
{
	if (!GetVisible())
	{
		bIsHovered = false;
		bIsPressed = false;
		bWasPressed = false;
		return;
	}

	// 슬라이드 업 애니메이션 이동 (Lerp)
	if (bIsSlideAnimating)
	{
		CurrentCenterY += (TargetCenterY - CurrentCenterY) * (SlideSpeed * deltaTime);
		if (std::abs(TargetCenterY - CurrentCenterY) < 0.5f)
		{
			CurrentCenterY = TargetCenterY;
			bIsSlideAnimating = false;
		}
		CenterY = CurrentCenterY;
	}

	if (!GetTouch())
	{
		bIsHovered = false;
		bIsPressed = false;
		bWasPressed = false;
		return;
	}

	// 마우스 호버 여부 갱신
	bIsHovered = IsPointInside(mouseX, mouseY);

	// 마우스 클릭 판정 (버튼 위에서 눌렀다가 뗐을 때 1회 호출)
	if (bIsHovered)
	{
		if (Global::bIsLButtonPressed && !bWasPressed)
		{
			bIsPressed = true;
		}
		else if (!Global::bIsLButtonPressed && bWasPressed && bIsPressed)
		{
			// 클릭 이벤트 콜백 실행
			if (OnClick)
			{
				OnClick();
			}
			bIsPressed = false;
		}
	}

	if (!Global::bIsLButtonPressed)
	{
		bIsPressed = false;
	}

	bWasPressed = Global::bIsLButtonPressed;

	// 목표 크기 계산 (눌림: 1.02배, 호버: 1.12배, 기본: 1.0배)
	if (bIsPressed)
	{
		TargetScale = PressScale;
	}
	else if (bIsHovered)
	{
		TargetScale = HoverScale;
	}
	else
	{
		TargetScale = 1.0f;
	}

	// 부드러운 스케일 보간 애니메이션 (Lerp)
	float lerpSpeed = 15.0f;
	CurrentScale += (TargetScale - CurrentScale) * (lerpSpeed * deltaTime);
	if (std::abs(TargetScale - CurrentScale) < 0.001f)
	{
		CurrentScale = TargetScale;
	}
}

void UUIButton::Update(float deltaTime)
{
	// Global에 실시간으로 저장되는 마우스 위치를 사용하여 호버/클릭 처리
	Update(deltaTime, Global::MouseScreenX, Global::MouseScreenY);
}

void UUIButton::Render(ID2D1RenderTarget* renderTarget)
{
	if (renderTarget && GetVisible())
	{
		float halfW = (BaseWidth * 0.5f) * CurrentScale;
		float halfH = (BaseHeight * 0.5f) * CurrentScale;
		float curY = bIsSlideAnimating ? CurrentCenterY : CenterY;

		if (ButtonBitmap)
		{
			D2D1_RECT_F drawRect = D2D1::RectF(
				CenterX - halfW,
				curY - halfH,
				CenterX + halfW,
				curY + halfH
			);

			renderTarget->DrawBitmap(
				ButtonBitmap,
				&drawRect,
				1.0f,
				D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
			);
		}

		if (!Text.empty() && TextFormat)
		{
			float textCenterX = CenterX + TextOffsetX;
			float textCenterY = curY + TextOffsetY;
			D2D1_RECT_F textRect = D2D1::RectF(
				textCenterX - halfW,
				textCenterY - halfH,
				textCenterX + halfW,
				textCenterY + halfH
			);

			ID2D1SolidColorBrush* textBrush = nullptr;
			if (SUCCEEDED(renderTarget->CreateSolidColorBrush(TextColor, &textBrush)))
			{
				renderTarget->DrawText(
					Text.c_str(),
					(UINT32)Text.length(),
					TextFormat,
					&textRect,
					textBrush
				);
				textBrush->Release();
			}
		}
	}
}
