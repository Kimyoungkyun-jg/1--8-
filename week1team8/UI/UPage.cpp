#include "UPage.h"
#include "UUIBackground.h"
#include "UFloatingText.h"
#include "UHUDText.h"
#include "UButton.h"

UUIPage::UUIPage(EPageType type)
	: PageType(type)
{
	SetVisible(false);
}

UUIPage::~UUIPage()
{
	for (UUIObject* obj : ChildUIObjects)
	{
		delete obj;
	}
	ChildUIObjects.clear();
}

void UUIPage::AddChild(UUIObject* uiObject)
{
	if (uiObject)
	{
		ChildUIObjects.push_back(uiObject);
	}
}

void UUIPage::Show()
{
	SetVisible(true);
	for (UUIObject* obj : ChildUIObjects)
	{
		if (obj) obj->SetVisible(true);
	}
}

void UUIPage::Hide()
{
	SetVisible(false);
	for (UUIObject* obj : ChildUIObjects)
	{
		if (obj) obj->SetVisible(false);
	}
}

void UUIPage::Update(float deltaTime)
{
	if (!GetVisible()) return;

	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;
	}
}



void UUIPage::Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font)
{
	if (!GetVisible() || !renderTarget) return;

	// 1. Backgrounds rendered first (bottom-most layer)
	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;

		if (UUIBackground* bg = dynamic_cast<UUIBackground*>(obj))
		{
			bg->Render(renderTarget);
		}
	}

	// 2. Foreground UI elements (HUD, Buttons, etc.)
	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;

		if (dynamic_cast<UUIBackground*>(obj)) continue;

		if (UUIHUDText* hud = dynamic_cast<UUIHUDText*>(obj))
		{
			hud->Render(renderTarget);
		}
		else if (UUIFloatingText* ft = dynamic_cast<UUIFloatingText*>(obj))
		{
			ft->Render(renderTarget, brush, font);
		}
		else if (UUIButton* btn = dynamic_cast<UUIButton*>(obj))
		{
			btn->Render(renderTarget);
		}
	}
}
