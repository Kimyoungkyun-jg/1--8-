#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <utility>
#include <cmath>
#include <algorithm>
#include "TotalManager.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace std;

struct FFloatingText
{
	float TargetScore; 
	float CurrentScore;  
	std::wstring CustomText;

	float X;
	float Y;
	float VelocityY; 
	float FloatTimer; //
	D2D1_COLOR_F Color;

	bool bIsFlyingToHUD; 
	bool bIsFinished; 

	static constexpr float TargetHUD_X = 50.0f; 
	static constexpr float TargetHUD_Y = 40.0f;

	FFloatingText()
		: TargetScore(0.0f)
		, CurrentScore(0.0f)
		, CustomText(L"")
		, X(0.0f)
		, Y(0.0f)
		, VelocityY(-30.0f)
		, FloatTimer(0.7f)
		, Color({ 1.0f, 1.0f, 1.0f, 1.0f })
		, bIsFlyingToHUD(false)
		, bIsFinished(false)
	{
	}

	FFloatingText(float score, float x, float y, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f })
		: TargetScore(score)
		, CurrentScore(0.0f)
		, CustomText(L"")
		, X(x)
		, Y(y)
		, VelocityY(-30.0f)
		, FloatTimer(0.7f)
		, Color(color)
		, bIsFlyingToHUD(false)
		, bIsFinished(false)
	{
	}

	FFloatingText(const wchar_t* text, float x, float y, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f })
		: TargetScore(0.0f)
		, CurrentScore(0.0f)
		, CustomText(text ? text : L"")
		, X(x)
		, Y(y)
		, VelocityY(-30.0f)
		, FloatTimer(0.7f)
		, Color(color)
		, bIsFlyingToHUD(false)
		, bIsFinished(false)
	{
	}

	void Update(float deltaTime)
	{
		if (!bIsFlyingToHUD)
		{
			Y += VelocityY * deltaTime;
			FloatTimer -= deltaTime;

			if (CurrentScore < TargetScore)
			{
				float speed = 15.0f;
				CurrentScore += (TargetScore - CurrentScore) * (speed * deltaTime);
				if (std::abs(TargetScore - CurrentScore) < 1.0f)
				{
					CurrentScore = TargetScore;
				}
			}

			if (FloatTimer <= 0.0f)
			{
				CurrentScore = TargetScore;
				bIsFlyingToHUD = true;
			}
		}
		else
		{
			float dx = TargetHUD_X - X;
			float dy = TargetHUD_Y - Y;
			float dist = std::sqrt(dx * dx + dy * dy);

			if (dist < 35.0f)
			{
				bIsFinished = true;
			}
			else
			{
				float flySpeed = 2400.0f * deltaTime;
				if (flySpeed > dist) flySpeed = dist;

				X += (dx / dist) * flySpeed;
				Y += (dy / dist) * flySpeed;
			}
		}
	}
};

class UIManager
{
public:
	static UIManager& Get()
	{
		static UIManager UIManager;
		return UIManager;
	}

	~UIManager();

	bool Initialize(IDXGISwapChain* swapChain);
	void Update(float deltaTime);
	void Render(int birdsLeft);
	void AddScore(int points);

	void SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f });
	void SpawnFloatingText(const wchar_t* text, float screenX, float screenY, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f });

	void Release();

	void CalPos(EColliderId colAId, EColliderId colBId, float colposx, float colposy, float hp);

private:
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;
	ID2D1RenderTarget* D2DRenderTarget = nullptr;
	ID2D1SolidColorBrush* Brush = nullptr;
	IDWriteTextFormat* HUDFont = nullptr;
	IDWriteTextFormat* FloatingFont = nullptr;

	int TargetScore = 0;
	float DisplayScore = 0.0f;
	vector<FFloatingText> FloatingTexts;
	bool bInitialized = false;

	vector<pair<float, float>> blockcolPoses;

private:
	UIManager() {};
	
};
