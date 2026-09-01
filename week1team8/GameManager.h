#pragma once

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
	}

	// Menu -> Exit
	void Exit()
	{

	}

private:
	GameManager() = default;
	~GameManager() = default;

	GameState state = GameState::Menu;
};

