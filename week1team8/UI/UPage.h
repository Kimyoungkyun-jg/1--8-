#pragma once

#include <vector>
#include <d2d1.h>
#include <dwrite.h>
#include "UUIObject.h"

enum class EPageType
{
	Starting = 0,
	InGame,
	Pause,
	Ending,
	StageClear
};

class UUIPage : public UUIObject
{
public:
	EPageType PageType;
	std::vector<UUIObject*> ChildUIObjects;

	UUIPage(EPageType type = EPageType::Starting);
	virtual ~UUIPage() override;

	void AddChild(UUIObject* uiObject);
	virtual void Show();
	virtual void Show(bool IsClear);
	virtual void Hide();
	virtual void Update(float deltaTime) override;
	virtual void Update(float deltaTime, float mouseX, float mouseY);
	virtual void Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font);

	virtual void OnMouseMove(float mouseX, float mouseY);
	virtual bool OnMouseDown(float mouseX, float mouseY);
	virtual void OnMouseUp(float mouseX, float mouseY);
};
