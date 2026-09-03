#pragma once

#include <d2d1.h>
#include <functional>
#include "UUIObject.h"


enum class EUIAnimType
{
	None,
	Lerp,
	Linear
};

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

	// Slide-up animation
	bool bUseSlideAnimation = false;
	bool bIsSlideAnimating = false;
	EUIAnimType AnimType = EUIAnimType::Lerp;
	float CurrentCenterY = 0.0f;
	float TargetCenterY = 0.0f;
	float StartOffsetY = 1200.0f;
	float SlideSpeed = 10.0f;

	// Button Text properties
	std::wstring Text = L"";
	float TextOffsetX = 0.0f;
	float TextOffsetY = 0.0f;
	D2D1_COLOR_F TextColor = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f);
	IDWriteTextFormat* TextFormat = nullptr;

	D2D1_RECT_F ButtonRect = D2D1::RectF(0.0f, 0.0f, 0.0f, 0.0f);
	ID2D1Bitmap* ButtonBitmap = nullptr;
	const wchar_t* imagePath = nullptr;

	std::function<void()> OnClick = nullptr;
	std::function<void()> OnAnimationFinished = nullptr;

public:
	UUIButton() = default;
	UUIButton(const wchar_t* url, float centerX, float centerY, float sizeX, float sizeY, bool bAnimate = false, float startOffsetY = 1200.0f, EUIAnimType animType = EUIAnimType::Lerp);
	virtual ~UUIButton() override;

	void SetText(const std::wstring& text, float offsetX = 0.0f, float offsetY = 0.0f, D2D1_COLOR_F color = D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f), float fontSize = 32.0f);
	void SetTextString(const std::wstring& text) { Text = text; }

	void SetImagePath(const wchar_t* path)
	{
		imagePath = path;
		ButtonBitmap = URenderer::GetInstance().LoadBitmapFromFile(imagePath);
	}

	void SetPoisition(float left, float top, float width, float height)
	{
		SetCenterPoisition(left + width * 0.5f, top + height * 0.5f, width, height);
	}

	void SetCenterPoisition(float centerX, float centerY, float sizeX, float sizeY)
	{
		CenterX = centerX;
		CenterY = centerY;
		BaseWidth = sizeX;
		BaseHeight = sizeY;
		TargetCenterY = centerY;
		CurrentCenterY = centerY;
		float halfW = sizeX * 0.5f;
		float halfH = sizeY * 0.5f;
		ButtonRect = D2D1::RectF(centerX - halfW, centerY - halfH, centerX + halfW, centerY + halfH);
	}

	bool IsPointInside(float x, float y) const;

	void SetSlideAnimation(bool bEnable, float startOffsetY = 1200.0f, float speed = 10.0f, EUIAnimType animType = EUIAnimType::Lerp);
	void SetLinearAnimation(float startY, float targetY, float speed = 150.0f);
	void ResetAnimation();
	void ResetAnimation(float startOffsetY);

	void OnMouseMove(float mouseX, float mouseY);
	bool OnMouseDown(float mouseX, float mouseY);
	void OnMouseUp(float mouseX, float mouseY);

	void SetOnClick(std::function<void()> callback) { OnClick = callback; }
	void SetOnAnimationFinished(std::function<void()> callback) { OnAnimationFinished = callback; }

	virtual void Update(float deltaTime) override;
	void Update(float deltaTime, float mouseX, float mouseY);
	void Render(ID2D1RenderTarget* renderTarget);
};
