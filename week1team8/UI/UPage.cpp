#include "UPage.h"
#include "UUIBackground.h"
#include "UFloatingText.h"
#include "UHUDText.h"
#include "UButton.h"
#include "../Global.h"

UUIPage::UUIPage(EPageType type)
	: PageType(type)
{
	ChildUIObjects.clear();
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

void UUIPage::Update(float deltaTime, float mouseX, float mouseY)
{
	if (!GetVisible()) return;

	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;
		if (UUIButton* btn = dynamic_cast<UUIButton*>(obj))
		{
			btn->Update(deltaTime, mouseX, mouseY);
		}
		else
		{
			obj->Update(deltaTime);
		}
	}
}

void UUIPage::Update(float deltaTime)
{
	Update(deltaTime, Global::MouseScreenX , Global::MouseScreenY);
}

void UUIPage::Render(ID2D1RenderTarget* renderTarget, ID2D1SolidColorBrush* brush, IDWriteTextFormat* font)
{
	if (!GetVisible() || !renderTarget) return;

	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;

		if (UUIBackground* bg = dynamic_cast<UUIBackground*>(obj))
		{
			bg->Render(renderTarget);
		}
	}

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

void UUIPage::OnMouseMove(float mouseX, float mouseY)
{
	if (!GetVisible()) return;

	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;
		if (UUIButton* btn = dynamic_cast<UUIButton*>(obj))
		{
			btn->OnMouseMove(mouseX, mouseY);
		}
	}
}

bool UUIPage::OnMouseDown(float mouseX, float mouseY)
{
	if (!GetVisible()) return false;

	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;
		if (UUIButton* btn = dynamic_cast<UUIButton*>(obj))
		{
			if (btn->OnMouseDown(mouseX, mouseY))
			{
				return true;
			}
		}
	}
	return false;
}

void UUIPage::OnMouseUp(float mouseX, float mouseY)
{
	if (!GetVisible()) return;

	for (UUIObject* obj : ChildUIObjects)
	{
		if (!obj || !obj->GetVisible()) continue;
		if (UUIButton* btn = dynamic_cast<UUIButton*>(obj))
		{
			btn->OnMouseUp(mouseX, mouseY);
		}
	}
}
