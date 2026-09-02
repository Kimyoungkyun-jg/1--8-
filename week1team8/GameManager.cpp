#include "GameManager.h"
#include "UObject.h"
#include "Global.h"
#include "TemplateLibrary.h"
#include "LoadManager.h"
#include "UI/UIManager.h"
#include <fstream>

void GameManager::Initialize()
{

}

void GameManager::Play()
{
	state = GameState::Play;
	UIManager::GetInstance().ChangePage(EPageType::InGame);
}

void GameManager::Pause()
{
	state = GameState::Pause;
	UIManager::GetInstance().ChangePage(EPageType::Pause);
}

void GameManager::Resume()
{
	state = GameState::Play;
	UIManager::GetInstance().ChangePage(EPageType::InGame);
}

void GameManager::Menu()
{
	state = GameState::Menu;
	UIManager::GetInstance().ChangePage(EPageType::Starting);
}

void GameManager::Exit()
{
	PostQuitMessage(0);
}

void GameManager::GameClear()
{
	state = GameState::GameClear;
	UIManager::GetInstance().GotoEnding(state);
}

void GameManager::Restart()
{
	PigCount = 0;
	Birds.clear();
	BirdTypes.clear();
	CurrentLevel = 0;
	SlingShot = nullptr;
	ReloadedBird = nullptr;
	UIManager::GetInstance().ResetScore();

	if (LoadManager::Get().LoadMap(0))
	{
		state = GameState::Play;
		return;
	}

	// 맵이 없어도 벽과 새총은 세운다. Play로는 안 넘어간다 (돼지 0마리 = 즉시 다음 레벨)
	LoadManager::Get().ClearMap();
	state = GameState::Menu;
}

void GameManager::SpawnWalls()
{
	// 빠른 물체가 한 프레임에 통과하지 않을 만큼의 두께
	const float thickness = 0.5f;

	// 셰이더가 x를 종횡비로 나누므로 보이는 x 범위가 곧 ±wAspectRatio다
	const float right = URenderer::GetInstance().wAspectRatio;
	const float left = -right;
	const float top = Global::topBorder;
	const float bottom = Global::bottomBorder;

	// 안쪽 면이 경계선에 맞도록 바깥으로 반 두께만큼 밀고, 모서리에서 서로 겹치게 키운다
	const float width = (right - left) + thickness * 2.0f;
	const float height = (top - bottom) + thickness * 2.0f;
	const float centerX = (left + right) * 0.5f;
	const float centerY = (top + bottom) * 0.5f;

	// 질량 0 = 정적
	SpawnColider<AGround>({ centerX, bottom - thickness * 0.5f, 0 }, EPrimitive::Rectangle, false, { width, thickness, 0 }, 0.0f);
	SpawnColider<AGround>({ centerX, top + thickness * 0.5f, 0 }, EPrimitive::Rectangle, false, { width, thickness, 0 }, 0.0f);
	SpawnColider<AGround>({ left - thickness * 0.5f, centerY, 0 }, EPrimitive::Rectangle, false, { thickness, height, 0 }, 0.0f);
	SpawnColider<AGround>({ right + thickness * 0.5f, centerY, 0 }, EPrimitive::Rectangle, false, { thickness, height, 0 }, 0.0f);
}

//새의 속도가 일정 이하면 호출
void GameManager::ReloadBird()
{
	if (Birds.size() > 0)
	{
		ReloadedBird = Birds.back();
		ReloadedBird->SetLocation(ShotPoint);
		ReloadedBird->SetVelocity(0.f);
		ReloadedBird->SetState(EBirdState::Idle);

		Birds.pop_back();
		SlingShot->EquippedBird = ReloadedBird;
		SlingShot->ShotPoint = ReloadedBird->GetLocation();
		ReloadedBird->SlingShot = SlingShot;
	}
	else
	{
		state = GameState::GameOver;
		UIManager::GetInstance().GotoEnding(state);
	}
}

ABird *GameManager::SpawnWaitingBird(FVector Location, EBirdType BirdType)
{
	ABird* Bird = nullptr;
	if (BirdType == EBirdType::Basic)
	{
		Bird = SpawnColider<ABird>(Location, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
		Bird->SetImage(L"Assets/img/bird.png");
		Birds.push_back(Bird);
	}
	else if (BirdType == EBirdType::BombBird)
	{
		Bird = SpawnColider<ABombBird>(Location, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
		Bird->SetImage(L"Assets/img/bomb.png");
		Birds.push_back(Bird);
	}
	else if (BirdType == EBirdType::FastBird)
	{
		Bird = SpawnColider<AFastBird>(Location, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
		Bird->SetImage(L"Assets/img/bird.png");
		Birds.push_back(Bird);
	}

	Bird->SetWait();
	return Bird;
}

void GameManager::SpawnBirdAndSlingShot()
{
	AActor* hill = SpawnActor<AActor>({ -1.2, -0.4, 0 }, EPrimitive::Rectangle, {1, 1.5, 1});
	hill->SetImage(L"Assets/img/hill.png");

	SlingShot = SpawnActor<ASlingShot>({ -1.18, -0.45, 0 }, EPrimitive::Rectangle, { 0.1, 0.2, 0 });
	SlingShot->SetImage(L"Assets/img/slingshot.png");
	SlingShot->SpawnBand();

	//대기하는 새들을 언덕에 스폰하여 배치한다. 언덕에 있는 새들은 클릭과 중력을 비활성화한다.
	FVector WaitPoint = ShotPoint;
	for (int i = 0; i < BirdTypes.size() - 1; ++i)
	{
		EBirdType BirdType = static_cast<EBirdType>(BirdTypes[i]);
		WaitPoint.x -= 0.1;
		SpawnWaitingBird(WaitPoint, BirdType);
	}

	//가장 뒤의 새를 새총에 배치한다.
	EBirdType BirdType = static_cast<EBirdType>(BirdTypes.back());
	ReloadedBird = SpawnWaitingBird(ShotPoint, BirdType);
	SlingShot->EquippedBird = ReloadedBird;
	SlingShot->ShotPoint = ReloadedBird->GetLocation();
	ReloadedBird->SlingShot = SlingShot;
	ReloadedBird->SetState(EBirdState::Idle);

	//Reloaded된 새는 제외
	Birds.pop_back();
}

void GameManager::PigDeath()
{
	--PigCount;
}

void GameManager::CheckGameState()
{
	if (state == GameState::Play)
	{
		if (PigCount == 0 && ReloadedBird->GetVelocity().Length() < 0.5f)
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
	}
}
