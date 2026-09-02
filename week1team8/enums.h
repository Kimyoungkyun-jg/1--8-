#pragma once
#include <string>

enum class EPrimitive
{
	Circle,
	Rectangle
};

enum class EColliderId
{
	BIRD,
	PIG,
	BLOCK,
	NONE
};

enum class GameState
{
	Start,
	Play,
	Pause,
	GameOver,
	GameClear
};