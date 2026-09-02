#pragma once

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
		Menu();
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

	void ReloadBird();
	void SpawnBirdAndSlingShot();
	ASlingShot* GetSlingShot() { return SlingShot; }
	ABird* GetReloadedBird() { return ReloadedBird; }
	void SetPigCount(int NewPigCount) { PigCount = NewPigCount; }
	void SetBirdCount(int NewBirdCount) { BirdCount = NewBirdCount; }
	int GetPigCount() const { return PigCount; }
	int GetBirdCount() const { return BirdCount; }
	GameState GetGameState() const { return state; }
	void PigDeath();
	void CheckGameState();

	int GetCurlvl() { return CurrentLevel; }

private:
	GameManager() = default;
	~GameManager() = default;

	ABird* ReloadedBird;
	ASlingShot* SlingShot;
	int PigCount = 0;
	int BirdCount = 0;
	int CurrentLevel = 0;

	GameState state = GameState::Menu;
};

