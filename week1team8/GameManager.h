#pragma once

#include <vector>
#include "Vector.h"
class ABird;
class ASlingShot;
class ACollider;

enum class GameState
{
	Menu,
	Play,
	Pause,
	StageClear,
	GameOver,
	GameClear,
	EndingCredit
};

enum EBirdType
{
	Basic,
	BombBird,
	FastBird
};

class GameManager
{
public:
	static GameManager& GetInstance()
	{
		static GameManager instance;
		return instance;
	}

	// 복사/이동 금지
	GameManager(const GameManager&) = delete;				// 복사 생성자
	GameManager& operator=(const GameManager&) = delete;	// 복사 대입 연산자
	GameManager(GameManager&&) = delete;					// 이동생성자
	GameManager& operator=(GameManager&&) = delete;			// 이동 대입 연산자

	/* 상태 전이 및 초기화 함수 */
	void Initialize();
	void Play();
	void Pause();
	void Resume();
	void Menu();
	void Exit();
	void GameClear();
	void Credit();

	// 맵을 세우는 데 성공했을 때만 true. 실패하면 스스로 메뉴로 되돌린다
	bool Restart();

	// 게임이 실제로 굴러가는 상태인지. 물리와 Tick은 이때만 진행한다
	bool IsSimulating() const { return state == GameState::Play; }
	void SpawnBirdAndSlingShot();
	void SpawnWalls();
	ASlingShot* GetSlingShot() { return SlingShot; }
	ABird* GetReloadedBird() { return ReloadedBird; }
	void SetPigCount(int NewPigCount) { PigCount = NewPigCount; }
	void SetBirdCount(int NewBirdCount)
	{
		BirdTypes.clear();
		for (int i = 0; i < NewBirdCount; i++)
		{
			int randi = rand() % 3;
			BirdTypes.push_back(randi);
		}
	}
	int GetPigCount() const { return PigCount; }
	int GetBirdCount() const { return Birds.size(); }

	// 대기 중인 새 + 새총에 올라간 새. HUD와 GameOver 판정이 봐야 하는 값
	int GetBirdsRemaining() const { return static_cast<int>(Birds.size()) + (ReloadedBird ? 1 : 0); }
	GameState GetGameState() const { return state; }
	void SetGameState(GameState gs) { state = gs; }
	void PigDeath();
	void CheckGameState();

	int GetCurlvl() { return CurrentLevel; }
	void SetCurlvl(int lvl) { CurrentLevel = lvl; }

	int GetMaxLevel() const { return Maxlevel; }
	void SetMaxLevel(int lvl) { Maxlevel = lvl; }

	void ReloadBird();

	// 콜라이더가 삭제되기 직전에 불린다. 매니저가 들고 있는 포인터를 끊는다
	void OnColliderDestroyed(ACollider* Destroyed);

	// 액터를 전부 지우기 전에 매니저 쪽 참조를 먼저 비운다
	void ClearRuntimeRefs();

private:
	GameManager() = default;
	~GameManager() = default;

	ABird* SpawnWaitingBird(FVector Location, EBirdType BirdType);

	ABird* ReloadedBird = nullptr;
	ASlingShot* SlingShot = nullptr;
	int PigCount = 0;
	int CurrentLevel = 0;
	int Maxlevel = 99;

	GameState state = GameState::Menu;
	
	std::vector<int> BirdTypes;
	std::vector<FVector> WaitPoints;
	std::vector<ABird*> Birds;

	FVector ShotPoint = { -1.18, -0.35, 0 };


public:
	bool IsClearLevel = false;
	bool bIsEditorMode = false;
	
};

