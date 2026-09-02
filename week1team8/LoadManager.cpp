#include "LoadManager.h"
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include "UObject.h"
#include "CollisionManager.h"
#include "TemplateLibrary.h"
#include "GameManager.h"

struct FSpawnInfo
{
	FVector Location;
	EPrimitive Primitive;
	FVector Scale;
	float Mass;
	EColliderId ColliderId;
};

static const std::string to_string(FVector v)
{
	return std::to_string(v.x) + " " + std::to_string(v.y) + " " + std::to_string(v.z);
}

const std::string MakeActorInfo(const AActor* Actor)
{
	return to_string(Actor->GetLocation()) + " " + std::to_string(static_cast<int>(Actor->GetPrimitive())) + " " + to_string(Actor->GetScale());
}

const std::string MakeColliderInfo(const ACollider *Collider)
{
	return MakeActorInfo(Collider) + " " + std::to_string(Collider->GetMass()) + " " + std::to_string(static_cast<int>(Collider->GetColliderId()));
}

void LoadManager::SaveMap(int BirdCount)
{
	//파일 하나를 생성 및 열기

	//파일에 정보 입력
	//Block_1의 스폰 정보(위치, 모양, 스케일, 질량)
	//Block_2의 스폰 정보...

	//x,y,z 0 1 0.05 0.2 20
	std::string filePath = "Map/map_" + std::to_string(MapNumber) + ".txt";
	std::filesystem::create_directories("Map");
	OutputDebugStringA(std::filesystem::absolute(filePath).string().c_str());
	std::ofstream newfile;
	newfile.open(filePath);

	//레벨에 스폰할 새 갯수
	newfile << BirdCount << '\n';

	if (newfile.is_open())
	{
		for (const ACollider* Obj : CollisionManager::GetInstance().colliders)
		{
			if (Obj->GetColliderId() == EColliderId::BLOCK || Obj->GetColliderId() == EColliderId::PIG)
			{
				const std::string BlockInfo = MakeColliderInfo(Obj);
				newfile << BlockInfo << '\n';
			}
		}

		++MapNumber;
	}

	newfile.close();
}

FSpawnInfo MakeSpawnInfo(std::string str, bool bIsActor)
{
	std::stringstream ss(str);

	FVector Loc;
	std::getline(ss, str, ' ');
	Loc.x = stof(str);
	std::getline(ss, str, ' ');
	Loc.y = stof(str);
	std::getline(ss, str, ' ');
	Loc.z = stof(str);

	EPrimitive Primitive = EPrimitive::Circle;
	std::getline(ss, str, ' ');
	switch (stoi(str))
	{
	case 0:
		Primitive = EPrimitive::Circle;
		break;
	case 1:
		Primitive = EPrimitive::Rectangle;
		break;
	default:
		break;
	}

	FVector Scale;
	std::getline(ss, str, ' ');
	Scale.x = stof(str);
	std::getline(ss, str, ' ');
	Scale.y = stof(str);
	std::getline(ss, str, ' ');
	Scale.z = stof(str);

	if (bIsActor)
		return { Loc, Primitive, Scale };

	float Mass;
	std::getline(ss, str, ' ');
	Mass = stof(str);

	EColliderId ColliderId = EColliderId::BIRD;
	std::getline(ss, str, ' ');
	switch (stoi(str))
	{
	case 0:
		ColliderId = EColliderId::BIRD;
		break;
	case 1:
		ColliderId = EColliderId::PIG;
		break;
	case 2:
		ColliderId = EColliderId::BLOCK;
		break;
	default:
		ColliderId = EColliderId::NONE;
		break;
	}

	return { Loc, Primitive, Scale, Mass, ColliderId };
}

bool LoadManager::LoadMap(int num)
{

	std::string filePath = "Map/map_" + std::to_string(num) + ".txt";
	std::ifstream savedfile;
	savedfile.open(filePath);

	if (savedfile.is_open())
	{
		if (GameManager::GetInstance().GetGameState() != GameState::Menu)
			ClearMap();

		std::string str;
		std::getline(savedfile, str);
		int BirdCount = std::stoi(str);
		GameManager::GetInstance().SetBirdCount(BirdCount);
		GameManager::GetInstance().SpawnBirdAndSlingShot();

		int PigCount = 0;
		while (getline(savedfile, str))
		{
			if (savedfile.eof()) break;
			FSpawnInfo ObstacleInfo = MakeSpawnInfo(str, false);
			if (ObstacleInfo.ColliderId == EColliderId::BLOCK)
			{
				ABlock* Block = SpawnColider<ABlock>(ObstacleInfo.Location, ObstacleInfo.Primitive, true, ObstacleInfo.Scale, ObstacleInfo.Mass);
				if (ObstacleInfo.Scale.x >= ObstacleInfo.Scale.y)
					Block->SetImage(L"Assets/img/plank.png");
				else
					Block->SetImage(L"Assets/img/plank_v.png");
			}
			else if (ObstacleInfo.ColliderId == EColliderId::PIG)
			{
				APig* pig = SpawnColider<APig>(ObstacleInfo.Location, ObstacleInfo.Primitive, true, ObstacleInfo.Scale, ObstacleInfo.Mass);
				pig->SetImage(L"Assets/img/pig.png");
				PigCount++;
			}
		}
		GameManager::GetInstance().SetPigCount(PigCount);
		return true;
	}

	return false;
}

void LoadManager::ClearMap()
{
	UObjectManager::GetInstance().DistroyAllActors();
	GameManager::GetInstance().SpawnWalls();
}
