#pragma once

#include "../UObject.h"

class UUIObject : public UObject
{
public:
	UUIObject();
	virtual ~UUIObject() override;

	void SetVisible(bool b) { bIsVisible = b; }
	bool GetVisible() const { return bIsVisible; }

	virtual void Update(float deltaTime) {}

	void SetTouch(bool b) { bCantouchable = b; }
	bool GetTouch() const { return bCantouchable; }

	int GetUIID() const { return UIIID; }

private:
	bool bIsVisible = true;
	bool bCantouchable = false;

	inline static int UIIDMax = 0;
	int UIIID;
};
