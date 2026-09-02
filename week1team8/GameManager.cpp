#include "GameManager.h"
#include "UObject.h"
#include "TemplateLibrary.h"

//새의 속도가 일정 이하면 호출
void GameManager::ReloadBird()
{
	ReloadedBird->Destroy();
	ReloadedBird = SpawnColider<ABird>({ -1.2, -0.2, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50);
	SlingShot->EquippedBird = ReloadedBird;
	ReloadedBird->SlingShot = SlingShot;
}

void GameManager::SpawnBirdAndSlingShot()
{
	SlingShot = SpawnActor<ASlingShot>({ -1.2, -0.6, 0 }, EPrimitive::Rectangle, { 0.05, 0.8, 0 });
	ReloadedBird = SpawnColider<ABird>({ -1.2, -0.2, 0 }, EPrimitive::Circle, false, { 0.1, 0.1, 0 }, 50);

	SlingShot->EquippedBird = ReloadedBird;
	SlingShot->ShotPoint = ReloadedBird->GetLocation();
	SlingShot->SpawnBand();
	ReloadedBird->SlingShot = SlingShot;
}
