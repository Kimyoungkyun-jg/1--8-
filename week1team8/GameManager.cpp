#include "GameManager.h"
#include "UObject.h"
#include "Global.h"
#include "TemplateLibrary.h"
#include "LoadManager.h"

void GameManager::SpawnWalls()
{
	// 두께. 빠른 물체가 한 프레임에 통과해버리지 않을 만큼은 있어야 한다.
	const float thickness = 0.5f;

	const float left = Global::leftBorder;
	const float right = Global::rightBorder;
	const float top = Global::topBorder;
	const float bottom = Global::bottomBorder;

	// 안쪽 면이 경계선에 딱 맞도록 경계 바깥으로 반 두께만큼 밀어서 놓는다.
	// 모서리에서 서로 겹치도록 가로/세로를 두께만큼씩 키운다.
	const float width = (right - left) + thickness * 2.0f;
	const float height = (top - bottom) + thickness * 2.0f;
	const float centerX = (left + right) * 0.5f;
	const float centerY = (top + bottom) * 0.5f;

	// 질량 0 = 정적. Move()가 바로 return하고, 솔버도 InvMass 0으로 보고 안 민다.
	SpawnColider<AGround>({ centerX, bottom - thickness * 0.5f, 0 }, EPrimitive::Rectangle, false, { width, thickness, 0 }, 0.0f);
	SpawnColider<AGround>({ centerX, top + thickness * 0.5f, 0 }, EPrimitive::Rectangle, false, { width, thickness, 0 }, 0.0f);
	SpawnColider<AGround>({ left - thickness * 0.5f, centerY, 0 }, EPrimitive::Rectangle, false, { thickness, height, 0 }, 0.0f);
	SpawnColider<AGround>({ right + thickness * 0.5f, centerY, 0 }, EPrimitive::Rectangle, false, { thickness, height, 0 }, 0.0f);
}

//새의 속도가 일정 이하면 호출
void GameManager::ReloadBird()
{
	ReloadedBird = SpawnColider<ABird>({ -1.2, -0.2, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
	SlingShot->EquippedBird = ReloadedBird;
	ReloadedBird->SlingShot = SlingShot;
}

void GameManager::SpawnBirdAndSlingShot()
{
	SlingShot = SpawnActor<ASlingShot>({ -1.2, -0.6, 0 }, EPrimitive::Rectangle, { 0.05, 0.8, 0 });
	ReloadedBird = SpawnColider<ABird>({ -1.2, -0.2, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);

	SlingShot->EquippedBird = ReloadedBird;
	SlingShot->ShotPoint = ReloadedBird->GetLocation();
	SlingShot->SpawnBand();
	ReloadedBird->SlingShot = SlingShot;
}

void GameManager::PigDeath()
{
	--PigCount;
}

void GameManager::CheckGameState()
{
	if (PigCount == 0)
	{
		if (LoadManager::Get().LoadMap(CurrentLevel + 1))
		{
			CurrentLevel += 1;
		}
		else
		{
			state = GameState::GameClear;
		}
	}
}
