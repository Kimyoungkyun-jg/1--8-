#include "UIManager.h"
#include <cmath>

UIManager::~UIManager()
{
	Release();
}

bool UIManager::Initialize(IDXGISwapChain* swapChain)
{
	if (!swapChain) return false;

	HRESULT hr;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
	if (FAILED(hr)) return false;

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&DWriteFactory));
	if (FAILED(hr)) return false;

	IDXGISurface* surface = nullptr;
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
	if (FAILED(hr)) return false;

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	hr = D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &D2DRenderTarget);
	surface->Release();
	if (FAILED(hr)) return false;

	hr = D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &Brush);
	if (FAILED(hr)) return false;

	// 고정 HUD용 폰트
	hr = DWriteFactory->CreateTextFormat(
		L"맑은 고딕",
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		26.0f,
		L"ko-kr",
		&HUDFont
	);
	if (FAILED(hr)) return false;

	// 튀어나오는 점수용 폰트
	hr = DWriteFactory->CreateTextFormat(
		L"맑은 고딕",
		nullptr,
		DWRITE_FONT_WEIGHT_EXTRA_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		32.0f,
		L"ko-kr",
		&FloatingFont
	);
	if (FAILED(hr)) return false;

	bInitialized = true;



	//디버그용 
	FFloatingText fttest;
	
	fttest.Text = L"100";
	fttest.X = 500.0f;
	fttest.Y = 500.0f;
	
	fttest.VelocityY = -30.0f;
	
	fttest.LifeTime = 3.0f;
	fttest.MaxLifeTime = 3.0f;
	
	fttest.Scale = 1.0f;
	
	fttest.Color = D2D1::ColorF(D2D1::ColorF::Yellow);
	
	FloatingTexts.push_back(fttest);



	return true;
}

void UIManager::AddScore(int points)
{
	TargetScore += points;
}

void UIManager::SpawnFloatingText(const wchar_t* text, float screenX, float screenY, D2D1_COLOR_F color)
{
	FFloatingText ft;
	ft.Text = text ? text : L"";
	ft.X = screenX;
	ft.Y = screenY;
	ft.VelocityY = -60.0f;
	ft.LifeTime = 1.2f;
	ft.MaxLifeTime = 1.2f;
	ft.Color = color;
	ft.Scale = 1.0f;

	FloatingTexts.push_back(ft);
}

void UIManager::Update(float deltaTime)
{
	// ① 점수 부드럽게 카운트업 (Lerp)
	if (DisplayScore < TargetScore)
	{
		float speed = 10.0f;
		DisplayScore += (TargetScore - DisplayScore) * (speed * deltaTime);
		if (std::abs(TargetScore - DisplayScore) < 1.0f)
		{
			DisplayScore = (float)TargetScore;
		}
	}

	// ② 플로팅 텍스트 이동 및 수명 깎기
	for (auto it = FloatingTexts.begin(); it != FloatingTexts.end(); )
	{
		it->Y += it->VelocityY * deltaTime;
		it->LifeTime -= deltaTime;

		if (it->LifeTime <= 0.0f)
		{
			it = FloatingTexts.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void UIManager::Render(int birdsLeft)
{
	if (!D2DRenderTarget)
		return;

	D2DRenderTarget->BeginDraw();


	// 상단 고정 HUD
	wchar_t scoreText[128];

	swprintf_s(
		scoreText,
		L"SCORE: %06d   BIRDS: %d",
		static_cast<int>(DisplayScore),
		birdsLeft
	);

	Brush->SetColor(
		D2D1::ColorF(D2D1::ColorF::White)
	);

	D2D1_RECT_F hudRect =
		D2D1::RectF(
			30.0f,
			30.0f,
			600.0f,
			80.0f
		);

	D2DRenderTarget->DrawText(
		scoreText,
		static_cast<UINT32>(wcslen(scoreText)),
		HUDFont,
		&hudRect,
		Brush,
		D2D1_DRAW_TEXT_OPTIONS_NONE,
		DWRITE_MEASURING_MODE_NATURAL
	);


	for (const FFloatingText& floatingText : FloatingTexts)
	{
		float alpha =
			floatingText.LifeTime /
			floatingText.MaxLifeTime;

		D2D1_COLOR_F drawColor =
			floatingText.Color;

		drawColor.a = alpha;

		Brush->SetColor(drawColor);

		D2D1_RECT_F rect =
			D2D1::RectF(
				floatingText.X,
				floatingText.Y,
				floatingText.X + 200.0f,
				floatingText.Y + 50.0f
			);

		D2DRenderTarget->DrawText(
			floatingText.Text.c_str(),
			static_cast<UINT32>(floatingText.Text.length()),
			FloatingFont,
			&rect,
			Brush,
			D2D1_DRAW_TEXT_OPTIONS_NONE,
			DWRITE_MEASURING_MODE_NATURAL
		);
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