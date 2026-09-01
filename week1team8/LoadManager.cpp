#include "LoadManager.h"
#include <fstream>
#include <string>
#include <sstream>
#include <filesystem>
#include "UObject.h"
#include "CollisionManager.h"

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
	return MakeActorInfo(Collider) + " " + std::to_string(Collider->GetMass());
}

void LoadManager::SaveMap(const ABird *Bird, const ASlingShot *SlingShot)
{
	//파일 하나를 생성 및 열기

	//파일에 정보 입력
	//모든 AActor의 스폰 정보(위치, 모양, 스케일, 질량)
	//SlingShot의 스폰 정보(위치, 모양, 스케일)
	//Block_1의 스폰 정보(위치, 모양, 스케일, 질량)
	//Block_2의 스폰 정보...

	//x,y,z 0 1 0.05 0.2 20
	std::string filePath = "Map\\map_" + std::to_string(MapNumber) + ".txt";
	std::filesystem::create_directories("Map");
	OutputDebugStringA(std::filesystem::absolute(filePath).string().c_str());
	std::ofstream newfile;
	newfile.open(filePath);

	if (newfile.is_open())
	{
		const std::string BirdInfo = MakeColliderInfo(Bird);
		const std::string SlingShotInfo = MakeActorInfo(SlingShot);
		newfile << BirdInfo << '\n' << SlingShotInfo << '\n';

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
