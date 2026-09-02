#pragma once

#include "UPage.h"
#include "UHUDText.h"
#include "UButton.h"
#include "UFloatingText.h"
#include "UUIBackground.h"
#include <vector>

class UUIIngamePage : public UUIPage
{
public:
	UUIIngamePage();
	virtual ~UUIIngamePage() override;

	bool Initialize(IDWriteFactory* dwriteFactory, ID2D1RenderTarget* d2dRenderTarget, ID2D1Bitmap* pauseBtnBitmap, ID2D1Bitmap* bgBitmap, int screenWidth, int screenHeight);

	void AddScore(int points);
	void ResetScore();
	void SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color = { 1.0f, 0.843f, 0.0f, 1.0f });

	void SetBirdsLeft(int count) { BirdsLeft = count; }
	int GetScore() const { return TargetScore; }

	virtual void Update(float deltaTime) override;
	virtual void Update(float deltaTime, float mouseX, float mouseY) override;
	virtual void Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font) override;
	virtual void Hide() override;

	void ClearFlowtingText();

public:
	UUIHUDText* InGameHUD = nullptr;
	UUIButton* PauseBtn = nullptr;
	std::vector<UUIFloatingText*> FloatingTexts;

	int TargetScore = 0;
	float DisplayScore = 0.0f;
	int BirdsLeft = 0;

private:
	void UpdateScore(float deltaTime);
	void UpdateFloatingTexts(float deltaTime);
};
