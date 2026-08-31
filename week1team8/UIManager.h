#pragma once

#include <windows.h>
#include <d3d11.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include "TotalManager.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

struct FFloatingText
{
	std::wstring Text;
	float X, Y;          // 현재 화면 위치
	float VelocityY;     // 위로 떠오르는 속도
	float LifeTime;      // 남은 시간
	float MaxLifeTime;   // 전체 수명
	float Scale;         // 글자 크기 비율
    D2D1::ColorF Color = D2D1::ColorF(D2D1::ColorF::White);
    
};


class UIManager : public TotalManager
{
    virtual ~UIManager() override;
public:
	bool Initialize(IDXGISwapChain* swapChain);

    void Update(float deltaTime);

    void Render(int birdsLeft);
    void AddScore(int points);


    //터진 위치에 텍스트 띄우기
    void SpawnFloatingText(const wchar_t* text, float locX, float loxY, D2D1::ColorF color = D2D1::ColorF(D2D1::ColorF::Gold));

    void Release();

private:
    ID2D1Factory* D2DFactory = nullptr;
    IDWriteFactory* DWriteFactory = nullptr;
    ID2D1RenderTarget* D2DRenderTarget = nullptr;
    ID2D1SolidColorBrush* Brush = nullptr;
    IDWriteTextFormat* HUDFont = nullptr;
    IDWriteTextFormat* FloatingFont = nullptr;

    // 점수 관련 변수
    int TargetScore = 0;       // 실제 최종 점수
    float DisplayScore = 0.0f; // 화면에 부드럽게 올라가는 표시 점수

    // 활성화된 플로팅 텍스트 목록
    std::vector<FFloatingText> FloatingTexts;

    bool bInitialized = false;
};
