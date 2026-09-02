#pragma once

#include "Vector.h"

namespace Global
{
	// 화면 경계 (NDC). x는 종횡비에 따라 달라져서 여기 없다 -> SpawnWalls 참고
	constexpr float topBorder = 1.0f;
	constexpr float bottomBorder = -1.0f;
	const FVector G(0.0f, -2.5f, 0.0f);

	// 실시간 마우스 위치 (스크린 픽셀 좌표 및 월드 좌표)
	inline float MouseScreenX = 0.0f;
	inline float MouseScreenY = 0.0f;
	inline FVector MouseWorldPos(0.0f, 0.0f, 0.0f);
	inline bool bIsLButtonPressed = false;
}
