#include "UUIObject.h"
#include "UIManager.h"
#include <algorithm>

UUIObject::UUIObject()
{
	++UIIDMax;
	UIIID = UIIDMax;
	UIManager::GetInstance().AllUIObjects.push_back(this);
}

UUIObject::~UUIObject()
{
	auto& list = UIManager::GetInstance().AllUIObjects;
	list.erase(std::remove(list.begin(), list.end(), this), list.end());
}
