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

	// 이전 판의 오브젝트를 남겨두면 메뉴 뒤에 계속 살아 있다
	LoadManager::Get().ClearMap();

	UIManager::GetInstance().ResetScore();
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

void GameManager::Credit()
{
	state = GameState::EndingCredit;
	UIManager::GetInstance().ChangePage(EPageType::EndingCredit);
}

void GameManager::ClearRuntimeRefs()
{
	// 여기서 delete하지 않는다. 실제 삭제는 UObjectManager가 한다
	Birds.clear();
	WaitPoints.clear();
	ReloadedBird = nullptr;
	SlingShot = nullptr;
	PigCount = 0;
}

bool GameManager::Restart()
{
	CurrentLevel = 0;

	// LoadMap이 ClearMap부터 부르므로 이전 판 정리는 그쪽에서 끝난다
	if (!LoadManager::Get().LoadMap(CurrentLevel))
	{
		// 맵이 없으면 Play로 넘어가면 안 된다 (돼지 0마리 = 즉시 클리어)
		Menu();
		return false;
	}

	// 남은 새 개수를 읽으므로 맵을 세운 뒤에 부른다
	UIManager::GetInstance().ResetScore();
	Play();
	return true;
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
	// 더 올릴 새가 없으면 손을 비운다. 이 상태라야 GameOver 판정이 성립한다
	if (Birds.empty())
	{
		ReloadedBird = nullptr;
		if (SlingShot)
		{
			SlingShot->EquippedBird = nullptr;
		}
		return;
	}

	ReloadedBird = Birds.back();
	ReloadedBird->SetLocation(ShotPoint);
	ReloadedBird->SetVelocity(0.f);
	ReloadedBird->SetState(EBirdState::Idle);

	Birds.pop_back();
	if (SlingShot)
	{
		SlingShot->EquippedBird = ReloadedBird;
		SlingShot->ShotPoint = ReloadedBird->GetLocation();
	}
	ReloadedBird->SlingShot = SlingShot;

	//웨이팅중인 새들을 한칸씩 땡긴다.
	for (int i = 0; i < Birds.size(); ++i)
	{
		Birds[i]->SetLocation(WaitPoints[Birds.size() - i - 1]);
	}
}

void GameManager::OnColliderDestroyed(ACollider* Destroyed)
{
	if (!Destroyed)
	{
		return;
	}

	// 대기열에 있던 새가 죽으면 목록에서 뺀다
	for (auto it = Birds.begin(); it != Birds.end(); ++it)
	{
		if (*it == Destroyed)
		{
			Birds.erase(it);
			break;
		}
	}

	if (SlingShot && SlingShot->EquippedBird == Destroyed)
	{
		SlingShot->EquippedBird = nullptr;
	}

	// 장전된 새가 사라지면 다음 새를 올린다. 없으면 ReloadBird가 nullptr로 만든다.
	// 이걸 안 하면 해제된 새를 계속 가리켜서 다음 클릭에 터진다
	if (ReloadedBird == Destroyed)
	{
		ReloadedBird = nullptr;
		ReloadBird();
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
		Bird = SpawnColider<ABombBird>(Location, EPrimitive::Circle, false, { 0.1, 0.13, 0 }, 50, -1);
		Bird->SetImage(L"Assets/img/bomb.png");
		Birds.push_back(Bird);
	}
	else if (BirdType == EBirdType::FastBird)
	{
		Bird = SpawnColider<AFastBird>(Location, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50, -1);
		Bird->SetImage(L"Assets/img/Fastbird.png");
		Birds.push_back(Bird);
	}

	Bird->SetWait();
	return Bird;
}

void GameManager::SpawnBirdAndSlingShot()
{
	Birds.clear();
	WaitPoints.clear();
	ReloadedBird = nullptr;

	AActor* hill = SpawnActor<AActor>({ -1.2, -0.4, 0 }, EPrimitive::Rectangle, {1, 1.5, 1});
	hill->SetImage(L"Assets/img/hill.png");

	SlingShot = SpawnActor<ASlingShot>({ -1.18, -0.45, 0 }, EPrimitive::Rectangle, { 0.1, 0.2, 0 });
	SlingShot->SetImage(L"Assets/img/slingshot.png");
	SlingShot->SpawnBand();

	// 맵의 새 개수가 0이면 아래 size() - 1이 size_t 언더플로를 일으킨다
	if (BirdTypes.empty())
	{
		return;
	}

	// 새총에 올릴 한 마리를 뺀 나머지가 언덕에서 대기한다
	const int WaitCount = static_cast<int>(BirdTypes.size()) - 1;

	//대기하는 새들을 언덕에 스폰하여 배치한다. 언덕에 있는 새들은 클릭과 중력을 비활성화한다.
	FVector WaitPoint = ShotPoint;
	WaitPoint.y -= 0.1;
	for (int i = 0; i < WaitCount; ++i)
	{
		WaitPoint.x -= 0.06;
		WaitPoint.y -= 0.08;
		WaitPoints.push_back(WaitPoint);
	}

	for (int i = 0; i < WaitCount; ++i)
	{
		EBirdType BirdType = static_cast<EBirdType>(BirdTypes[i]);
		SpawnWaitingBird(WaitPoints[WaitCount - i - 1], BirdType);
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
	if (state == GameState::Play && CollisionManager::GetInstance().bIsAllStop)
	{
		if (PigCount == 0)
		{	
			state = GameState::StageClear;
			IsClearLevel = true;
			UIManager::GetInstance().LevelChanged(CurrentLevel);
		}
		// 새총에 올라간 새는 Birds에 없다. ReloadedBird까지 비어야 진짜 다 쓴 것
		else if (PigCount > 0 && Birds.empty() && ReloadedBird == nullptr)
		{
			state = GameState::GameOver;
			UIManager::GetInstance().GotoEnding(state);
		}
	}
}
