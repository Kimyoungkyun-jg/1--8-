#include "UIManager.h"

#include "UIManager.h"
#include <cmath>



UIManager::~UIManager()
{
    Release();
}

bool UIManager::Initialize(IDXGISwapChain* swapChain)
{
    if (!swapChain) return false;

    D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&DWriteFactory);

    IDXGISurface* surface = nullptr;
    swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));

    D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_DEFAULT,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
    );

    D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &D2DRenderTarget);
    surface->Release();

    D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &Brush);

    //고정 HUD용 폰트
    DWriteFactory->CreateTextFormat(L"맑은 고딕", nullptr, DWRITE_FONT_WEIGHT_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 26.0f, L"ko-kr", &HUDFont);

    //튀어나오는 점수용 폰트
    DWriteFactory->CreateTextFormat(L"맑은 고딕", nullptr, DWRITE_FONT_WEIGHT_EXTRA_BOLD,
        DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, 32.0f, L"ko-kr", &FloatingFont);

    bInitialized = true;
    return true;
}

//증가
void UIManager::AddScore(int points)
{
    TargetScore += points;
}

// 
void UIManager::SpawnFloatingText(const wchar_t* text, float screenX, float screenY, D2D1::ColorF color)
{
    FFloatingText ft;
    ft.Text = text;
    ft.X = screenX;
    ft.Y = screenY;
    ft.VelocityY = -60.0f; // 초당 위로 60픽셀씩 이동
    ft.LifeTime = 1.2f;    // 1.2초 동안 지속
    ft.MaxLifeTime = 1.2f;
    ft.Color = color;
    ft.Scale = 1.0f;

    FloatingTexts.push_back(ft);
}

// ⭐️ 3. 애니메이션 및 수명 업데이트
void UIManager::Update(float deltaTime) // deltaTime: 초 단위 (예: 0.016s)
{
    // ① 점수 부드럽게 카운트업 (Lerp)
    if (DisplayScore < TargetScore)
    {
        float speed = 10.0f; // 카운트업 속도
        DisplayScore += (TargetScore - DisplayScore) * (speed * deltaTime);
        if (std::abs(TargetScore - DisplayScore) < 1.0f)
        {
            DisplayScore = (float)TargetScore;
        }
    }

    // ② 플로팅 텍스트 이동 및 수명 깎기
    for (auto it = FloatingTexts.begin(); it != FloatingTexts.end(); )
    {
        it->Y += it->VelocityY * deltaTime; // 위로 상승
        it->LifeTime -= deltaTime;          // 시간 감소

        if (it->LifeTime <= 0.0f)
        {
            it = FloatingTexts.erase(it); // 시간 다 되면 삭제
        }
        else
        {
            ++it;
        }
    }
}

// ⭐️ 4. 화면 그리기
void UIManager::Render(int birdsLeft)
{
    if (!D2DRenderTarget) return;

    D2DRenderTarget->BeginDraw();

    
    //상단 고정 HUD (점수판 및 남은 새)
    wchar_t scoreText[128];
    swprintf_s(scoreText, L"SCORE: %06d   BIRDS: %d", (int)DisplayScore, birdsLeft);

    Brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
    D2DRenderTarget->DrawText(
        scoreText, wcslen(scoreText), HUDFont,
        D2D1::RectF(30.0f, 30.0f, 600.0f, 80.0f), Brush
    );


    //움직이는 플로팅 텍스트 렌더링 (페이드 아웃 적용)
    for (const auto& ft : FloatingTexts)
    {
        // 남은 시간에 따라 투명도(알파값) 조절
        float alpha = ft.LifeTime / ft.MaxLifeTime;
        D2D1::ColorF drawColor = ft.Color;
        drawColor.a = alpha; // 서서히 투명해짐

        Brush->SetColor(drawColor);

        D2D1_RECT_F rect = D2D1::RectF(ft.X, ft.Y, ft.X + 200.0f, ft.Y + 50.0f);
        D2DRenderTarget->DrawText(ft.Text.c_str(), ft.Text.length(), FloatingFont, rect, Brush);
    }

    D2DRenderTarget->EndDraw();
}

void UIManager::Release()
{
    if (FloatingFont) { FloatingFont->Release(); FloatingFont = nullptr; }
    if (HUDFont) { HUDFont->Release(); HUDFont = nullptr; }
    if (Brush) { Brush->Release(); Brush = nullptr; }
    if (D2DRenderTarget) { D2DRenderTarget->Release(); D2DRenderTarget = nullptr; }
    if (DWriteFactory) { DWriteFactory->Release(); DWriteFactory = nullptr; }
    if (D2DFactory) { D2DFactory->Release(); D2DFactory = nullptr; }
    bInitialized = false;
}