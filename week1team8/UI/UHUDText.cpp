#include "UHUDText.h"
#include <cwchar>

UUIHUDText::UUIHUDText(float x, float y, float width, float height)
	: X(x), Y(y), Width(width), Height(height), DisplayScore(0.0f), BirdsLeft(0)
{
}

UUIHUDText::~UUIHUDText()
{
	if (HUDFont) { HUDFont->Release(); HUDFont = nullptr; }
	if (HUDBrush) { HUDBrush->Release(); HUDBrush = nullptr; }
}

void UUIHUDText::SetData(float displayScore, int birdsLeft)
{
	DisplayScore = displayScore;
	BirdsLeft = birdsLeft;
	SetPosition(60, 80);
}

void UUIHUDText::SetPosition(float x, float y)
{
	X = x;
	Y = y;
}

bool UUIHUDText::Initialize(IDWriteFactory* dwriteFactory, ID2D1RenderTarget* renderTarget)
{
	if (!dwriteFactory || !renderTarget) return false;

	HRESULT hr = dwriteFactory->CreateTextFormat(
		L"Jalnan Gothic TTF",
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_ITALIC,
		DWRITE_FONT_STRETCH_NORMAL,
		36.0f,
		L"ko-kr",
		&HUDFont
	);
	if (FAILED(hr)) return false;

	hr = renderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Yellow), &HUDBrush);
	if (FAILED(hr)) return false;

	return true;
}

void UUIHUDText::Render(ID2D1RenderTarget* renderTarget)
{
	if (!renderTarget || !HUDBrush || !HUDFont) return;

	wchar_t scoreText[128];
	swprintf_s(
		scoreText,
		L"SCORE: %06d   BIRDS: %d",
		static_cast<int>(DisplayScore),
		BirdsLeft
	);

	HUDBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));
	D2D1_RECT_F hudRect = D2D1::RectF(X, Y, X + Width, Y + Height);

	renderTarget->DrawText(
		scoreText,
		static_cast<UINT32>(wcslen(scoreText)),
		HUDFont,
		&hudRect,
		HUDBrush,
		D2D1_DRAW_TEXT_OPTIONS_NONE,
		DWRITE_MEASURING_MODE_NATURAL
	);
}

void UUIHUDText::Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font)
{
	if (!renderTarget) return;

	ID2D1SolidColorBrush* useBrush = brush ? brush : HUDBrush;
	IDWriteTextFormat* useFont = font ? font : HUDFont;

	if (!useBrush || !useFont) return;

	wchar_t scoreText[128];
	swprintf_s(
		scoreText,
		L"SCORE: %06d   BIRDS: %d",
		static_cast<int>(DisplayScore),
		BirdsLeft
	);

	useBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Yellow));
	D2D1_RECT_F hudRect = D2D1::RectF(X, Y, X + Width, Y + Height);

	renderTarget->DrawText(
		scoreText,
		static_cast<UINT32>(wcslen(scoreText)),
		useFont,
		&hudRect,
		useBrush,
		D2D1_DRAW_TEXT_OPTIONS_NONE,
		DWRITE_MEASURING_MODE_NATURAL
	);
}
