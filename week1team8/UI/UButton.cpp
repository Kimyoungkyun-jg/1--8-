#include "UButton.h"
#include "UIManager.h"
#include "../Global.h"
#include <cmath>

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

bool UUIButton::IsPointInside(float x, float y) const
{
	float halfW = (BaseWidth * 0.5f) * CurrentScale;
	float halfH = (BaseHeight * 0.5f) * CurrentScale;
	return (x >= CenterX - halfW && x <= CenterX + halfW &&
	        y >= CenterY - halfH && y <= CenterY + halfH);
}

void UUIButton::OnMouseMove(float mouseX, float mouseY)
{
	if (!GetVisible())
	{
		bIsHovered = false;
		return;
	}

	bIsHovered = IsPointInside(mouseX, mouseY);
}

bool UUIButton::OnMouseDown(float mouseX, float mouseY)
{
	if (!GetVisible()) return false;

	if (IsPointInside(mouseX, mouseY))
	{
		if (OnClick)
		{
			OnClick();
		}
		return true;
	}
	return false;
}

void UUIButton::OnMouseUp(float mouseX, float mouseY)
{
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
	if (ButtonBitmap && renderTarget && GetVisible())
	{
		// 중심점(CenterX, CenterY)을 기준으로 확대/축소 사각형 계산
		float halfW = (BaseWidth * 0.5f) * CurrentScale;
		float halfH = (BaseHeight * 0.5f) * CurrentScale;
		D2D1_RECT_F drawRect = D2D1::RectF(
			CenterX - halfW,
			CenterY - halfH,
			CenterX + halfW,
			CenterY + halfH
		);

		renderTarget->DrawBitmap(
			ButtonBitmap,
			&drawRect,
			1.0f,
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}
}
