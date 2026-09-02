#include "GameManager.h"
#include "UObject.h"
#include "TemplateLibrary.h"
#include "LoadManager.h"
#include "UI/UIManager.h"

void GameManager::Initialize()
{
	Menu();
}

void GameManager::Menu()
{
	state = GameState::Start;
	UIManager::GetInstance().ResetScore();
	UIManager::GetInstance().ChangePage(EPageType::Starting);
}

void GameManager::Restart()
{
	PigCount = 0;
	BirdCount = 0;
	CurrentLevel = 0;
	SlingShot = nullptr;
	ReloadedBird = nullptr;

	state = GameState::Play;
	UIManager::GetInstance().ResetScore();
	LoadManager::Get().LoadMap(0);
}

// 새의 속도가 일정 이하면 호출
void GameManager::ReloadBird()
{
	if (state != GameState::Play) return;

	if (BirdCount > 0)
	{
		ReloadedBird = SpawnColider<ABird>({ -1.2, -0.2, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
		ReloadedBird->SetImage(L"Assets/img/bird.png");
		SlingShot->EquippedBird = ReloadedBird;
		ReloadedBird->SlingShot = SlingShot;
	}
	--BirdCount;
}

void GameManager::SpawnBirdAndSlingShot()
{
	SlingShot = SpawnActor<ASlingShot>({ -1.2, -0.6, 0 }, EPrimitive::Rectangle, { 0.05, 0.8, 0 });
	SlingShot->SetImage(L"Assets/img/slingshot.png");

	ReloadedBird = SpawnColider<ABird>({ -1.2, -0.2, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
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
		if (PigCount == 0)  // 다음 레벨 넘어가는 조건
		{
			if (LoadManager::Get().LoadMap(CurrentLevel + 1))
			{
				CurrentLevel += 1;
				UIManager::GetInstance().LevelChanged(CurrentLevel);
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
