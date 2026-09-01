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
#include "TotalManager.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

using namespace std;

struct FFloatingText
{
	float TargetScore = 0.0f;
	float CurrentScore = 0.0f;
	std::wstring CustomText = L"";

	float X = 0.0f;
	float Y = 0.0f;
	float VelocityY = -60.0f;
	float LifeTime = 1.2f;
	float MaxLifeTime = 1.2f;
	float Scale = 1.0f;
	D2D1_COLOR_F Color = { 1.0f, 0.843f, 0.0f, 1.0f }; // Gold

	void* TargetID = nullptr;

	FFloatingText()
		: Color({ 1.0f, 1.0f, 1.0f, 1.0f })
	{
	}

	FFloatingText(float score, float x, float y, void* target = nullptr, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f })
		: TargetScore(score)
		, CurrentScore(0.0f)
		, X(x)
		, Y(y)
		, VelocityY(-60.0f)
		, LifeTime(1.2f)
		, MaxLifeTime(1.2f)
		, Scale(1.0f)
		, Color(color)
		, TargetID(target)
	{
	}

	FFloatingText(const wchar_t* text, float x, float y, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f })
		: CustomText(text ? text : L"")
		, X(x)
		, Y(y)
		, VelocityY(-60.0f)
		, LifeTime(1.2f)
		, MaxLifeTime(1.2f)
		, Scale(1.0f)
		, Color(color)
	{
	}

	void AddScore(float additionalScore)
	{
		TargetScore += additionalScore;
		LifeTime = MaxLifeTime;
		Scale = 1.2f;
	}

	void Update(float deltaTime)
	{
		Y += VelocityY * deltaTime;
		LifeTime -= deltaTime;

		if (CurrentScore < TargetScore)
		{
			float speed = 15.0f;
			CurrentScore += (TargetScore - CurrentScore) * (speed * deltaTime);
			if (std::abs(TargetScore - CurrentScore) < 1.0f)
			{
				CurrentScore = TargetScore;
			}
		}

		if (Scale > 1.0f)
		{
			Scale -= 1.0f * deltaTime;
		}
	}
};

class UIManager
{
public:
	UIManager() {};

	static UIManager& GetInstance() // 싱글톤 패턴으로 관리
	{
		static UIManager instance;
		return instance;
	}

	UIManager(const UIManager&) = delete;
	UIManager& operator=(const UIManager&) = delete;

protected:


	bool Initialize(IDXGISwapChain* swapChain);
	void Update(float deltaTime);
	void Render(int birdsLeft);
	void AddScore(int points);

	void SpawnFloatingText(float score, float screenX, float screenY, void* targetID = nullptr, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f });
	void SpawnFloatingText(const wchar_t* text, float screenX, float screenY, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f });

	void Release();

	void CalPos(EColliderId colAId, EColliderId colBId, float colposx, float colposy, float hp, void* targetID = nullptr);
	void CalScore();

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
};
