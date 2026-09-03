#pragma once

#include <d2d1.h>
#include <dwrite.h>
#include "UUIObject.h"

class UUIHUDText : public UUIObject
{
public:
	UUIHUDText(float x, float y, float width = 800.0f, float height = 100.0f);
	virtual ~UUIHUDText() override;

	void SetData(float displayScore, int birdsLeft);
	void SetPosition(float x, float y);
	bool Initialize(IDWriteFactory* dwriteFactory, ID2D1RenderTarget* renderTarget);
	void Render(ID2D1RenderTarget* renderTarget);
	void Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font);

private:
	float X;
	float Y;
	float Width;
	float Height;
	float DisplayScore;
	int BirdsLeft;

	IDWriteTextFormat* HUDFont = nullptr;
	ID2D1SolidColorBrush* HUDBrush = nullptr;
};
