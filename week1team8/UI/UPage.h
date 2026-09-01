#pragma once

#include <vector>
#include <d2d1.h>
#include <dwrite.h>
#include "UUIObject.h"

enum class EPageType
{
	Starting= 0,
	InGame,
	Pause,
	Ending
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
	virtual void Hide();
	virtual void Update(float deltaTime);
	virtual void Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font);
};
