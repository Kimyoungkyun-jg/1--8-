#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include "TotalManager.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

struct FFloatingText
{
	std::wstring Text;
	float X;
	float Y;
	float VelocityY;
	float LifeTime;
	float MaxLifeTime;
	float Scale;
	D2D1_COLOR_F Color;

	FFloatingText()
		: Text(L"")
		, X(0.0f)
		, Y(0.0f)
		, VelocityY(-60.0f)
		, LifeTime(1.2f)
		, MaxLifeTime(1.2f)
		, Scale(1.0f)
		, Color(D2D1::ColorF(D2D1::ColorF::White))
	{
	}
};

class UIManager : public TotalManager
{
public:
	UIManager() : TotalManager() {}
	virtual ~UIManager() override;

	bool Initialize(IDXGISwapChain* swapChain);
	void Update(float deltaTime);
	void Render(int birdsLeft);
	void AddScore(int points);
	void SpawnFloatingText(const wchar_t* text, float screenX, float screenY, D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Gold));
	void Release();

private:
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;
	ID2D1RenderTarget* D2DRenderTarget = nullptr;
	ID2D1SolidColorBrush* Brush = nullptr;
	IDWriteTextFormat* HUDFont = nullptr;
	IDWriteTextFormat* FloatingFont = nullptr;

	int TargetScore = 0;
	float DisplayScore = 0.0f;
	std::vector<FFloatingText> FloatingTexts;
	bool bInitialized = false;
};
