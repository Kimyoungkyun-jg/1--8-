#pragma once

#include <windows.h>
#include <wincodec.h>
#include <d3d11.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <utility>
#include <cmath>
#include <algorithm>
#include <unordered_map>
#include "../CollisionManager.h"
#include "UUIObject.h"
#include "UPage.h"
#include "UUIIngamePage.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

using namespace std;

class UIManager
{
public:
	static UIManager& GetInstance()
	{
		static UIManager instance;
		return instance;
	}

	~UIManager();

	bool Initialize(IDXGISwapChain* swapChain, int nWidth, int nHeight);
	void Update(float deltaTime);
	void Render(int birdsLeft);
	void AddScore(int points);

	void SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f });
	void Release();

	void CalPos(EColliderId colAId, EColliderId colBId, float colposx, float colposy);
	void GetCollisionInfos(std::vector<CollisionInfo> infos);

	void ChangePage(EPageType newPageType)
	{
		if (CurrentPage)
		{
			CurrentPage->Hide();
		}

		auto it = Pages.find(newPageType);
		if (it != Pages.end())
		{
			CurrentPage = it->second;
			CurrentPage->Show();
		}
	}

	UUIPage* GetPage(EPageType type)
	{
		auto it = Pages.find(type);
		return (it != Pages.end()) ? it->second : nullptr;
	}

	ID2D1Bitmap* LoadBitmapFromFile(const wchar_t* uri);

public:
	UUIPage* CurrentPage = nullptr;
	unordered_map<EPageType, UUIPage*> Pages;
	vector<UUIObject*> AllUIObjects;



private:
	ID2D1Factory* D2DFactory = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;
	ID2D1RenderTarget* D2DRenderTarget = nullptr;
	ID2D1SolidColorBrush* Brush = nullptr;
	IDWriteTextFormat* FloatingFont = nullptr;
	IWICImagingFactory* WICFactory = nullptr;

	int screenWidth = 0;
	int screenHeight = 0;

private:
	UIManager() {};
	std::pair<float, float> WorldToScreen(const FVector& worldPos);
	
};
