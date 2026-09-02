#pragma once

#include <d2d1.h>
#include <functional>
#include "UUIObject.h"

class UUIButton : public UUIObject
{
public:
	// Button dimensions and center
	float CenterX = 0.0f;
	float CenterY = 0.0f;
	float BaseWidth = 0.0f;
	float BaseHeight = 0.0f;

	// Mouse hover & click animation
	float CurrentScale = 1.0f;
	float TargetScale = 1.0f;
	float HoverScale = 1.12f;
	float PressScale = 1.02f;
	bool bIsHovered = false;
	bool bIsPressed = false;
	bool bWasPressed = false;

	D2D1_RECT_F ButtonRect = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
	ID2D1Bitmap* ButtonBitmap = nullptr;
	const wchar_t* imagePath = nullptr;

	// 클릭 시 실행할 함수 포인터 / 콜백
	std::function<void()> OnClick = nullptr;

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
		BaseWidth = width;
		BaseHeight = height;
		CenterX = left + width * 0.5f;
		CenterY = top + height * 0.5f;
		ButtonRect = D2D1::RectF(left, top, left + width, top + height);
	}

	void SetCenterPoisition(float centerX, float centerY, float sizeX, float sizeY)
	{
		CenterX = centerX;
		CenterY = centerY;
		BaseWidth = sizeX;
		BaseHeight = sizeY;
		float halfW = sizeX * 0.5f;
		float halfH = sizeY * 0.5f;
		ButtonRect = D2D1::RectF(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
	}

	bool IsPointInside(float x, float y) const;

	void OnMouseMove(float mouseX, float mouseY);
	bool OnMouseDown(float mouseX, float mouseY);
	void OnMouseUp(float mouseX, float mouseY);

	void SetOnClick(std::function<void()> callback) { OnClick = callback; }

	virtual void Update(float deltaTime) override;
	void Update(float deltaTime, float mouseX, float mouseY);
	void Render(ID2D1RenderTarget* renderTarget);
};
