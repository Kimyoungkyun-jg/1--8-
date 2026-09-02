#pragma once

#include <vector>
#include "Vector.h"
class ABird;
class ASlingShot;

enum class GameState
{
	Menu,
	Play,
	Pause,
	GameOver,
	GameClear
};

enum EBirdType
{
	Basic,
	BombBird
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

	void Initialize()
	{
		Play();
	}

	// Menu -> Play
	void Play()
	{
		state = GameState::Play;
	}

	// Play -> Pause
	void Pause()
	{
		state = GameState::Pause;
	}

	// Pause -> Play
	void Resume()
	{
		state = GameState::Play;
	}

	// Pause -> Menu
	void Menu()
	{
		state = GameState::Menu;
		// UIManager::GetInstance().Menu()
	}

	// Menu -> Exit
	void Exit()
	{
		// 프로그램 종료
	}

	void Restart();
	void SpawnBirdAndSlingShot();
	void SpawnWalls();
	ASlingShot* GetSlingShot() { return SlingShot; }
	ABird* GetReloadedBird() { return ReloadedBird; }
	void SetPigCount(int NewPigCount) { PigCount = NewPigCount; }
	void SetBirdCount(int NewBirdCount)
	{
		for (int i = 0; i < NewBirdCount; i++)
		{
			int randi = rand() % 2;
			BirdTypes.push_back(randi);
		}
	}
	int GetPigCount() const { return PigCount; }
	int GetBirdCount() const { return Birds.size(); }
	GameState GetGameState() const { return state; }
	void PigDeath();
	void CheckGameState();

	int GetCurlvl() { return CurrentLevel; }

	void ReloadBird();

private:
	GameManager() = default;
	~GameManager() = default;

	ABird * SpawnWaitingBird(FVector Location, EBirdType BirdType);

	ABird* ReloadedBird = nullptr;
	ASlingShot* SlingShot = nullptr;
	int PigCount = 0;
	int CurrentLevel = 0;

	GameState state = GameState::Menu;
	
	std::vector<int> BirdTypes;
	std::vector<ABird*> Birds;

	FVector ShotPoint = { -1.18, -0.35, 0 };
};

