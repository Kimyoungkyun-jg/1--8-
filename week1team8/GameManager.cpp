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

	if (LoadManager::Get().LoadMap(0))
	{
		state = GameState::Play;
		return;
	}

	// 맵 파일이 없어도 벽과 새총은 세워둔다 (SlingShot이 null로 남으면 안 된다).
	// 다만 Play로는 안 넘어간다. 돼지가 0마리라 CheckGameState가 바로 다음 레벨을 찾는다.
	LoadManager::Get().ClearMap();
	state = GameState::Menu;
}

void GameManager::SpawnWalls()
{
	// 빠른 물체가 한 프레임에 통과하지 않을 만큼의 두께
	const float thickness = 0.5f;

	// 셰이더가 x를 종횡비로 나누므로, 보이는 x 범위가 곧 ±wAspectRatio다.
	// 고정값을 쓰면 벽이 화면 밖이나 안쪽에 생긴다.
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
