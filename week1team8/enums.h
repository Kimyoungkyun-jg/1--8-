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
	NONE,
	GROUND	// 화면 경계 벽. 저장된 맵 파일과 호환되도록 맨 뒤에 둔다.
};