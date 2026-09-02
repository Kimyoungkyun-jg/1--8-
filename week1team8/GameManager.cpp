#include "GameManager.h"
#include "UObject.h"
#include "Global.h"
#include "TemplateLibrary.h"
#include "LoadManager.h"
#include "UI/UIManager.h"

void GameManager::Restart()
{
	PigCount = 0;
	BirdCount = 0;
	CurrentLevel = 0;
	SlingShot = nullptr;
	ReloadedBird = nullptr;

	state = GameState::Play;
	LoadManager::Get().LoadMap(0);
}

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
	if (BirdCount > 0)
	{
		ReloadedBird = SpawnColider<ABird>({ -1.18, -0.35, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
		ReloadedBird->SetImage(L"Assets/img/bird.png");
		SlingShot->EquippedBird = ReloadedBird;
		ReloadedBird->SlingShot = SlingShot;
	}
	--BirdCount;
}

void GameManager::SpawnBirdAndSlingShot()
{
	AActor* hill = SpawnActor<AActor>({ -1.2, -0.4, 0 }, EPrimitive::Rectangle, {1, 1.5, 1});
	hill->SetImage(L"Assets/img/hill.png");

	SlingShot = SpawnActor<ASlingShot>({ -1.18, -0.45, 0 }, EPrimitive::Rectangle, { 0.1, 0.2, 0 });
	SlingShot->SetImage(L"Assets/img/slingshot.png");

	ReloadedBird = SpawnColider<ABird>({ -1.18, -0.35, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
	ReloadedBird->SetImage(L"Assets/img/bird.png");

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
	if (state == GameState::Play)
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
				UIManager::GetInstance().GotoEnding(state);
			}
		}

		if (BirdCount == -1)
		{
			state = GameState::GameOver;
			UIManager::GetInstance().GotoEnding(state);
		}
	}
}
