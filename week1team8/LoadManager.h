#pragma once
#include "Vector.h"
#include "UObject.h"

class ABird;
class ASlingShot;

struct FSpawnInfo
{
	FVector Location;
	EPrimitive Primitive;
	FVector Scale;
	float Mass;
	EColliderId ColliderId;
	EBlockType BlockType;

	FSpawnInfo(FVector _Location,
		EPrimitive _Primitive,
		FVector _Scale,
		float _Mass,
		EColliderId _ColliderId,
		EBlockType _BlockType) : Location(_Location), Primitive(_Primitive), Scale(_Scale), Mass(_Mass), ColliderId(_ColliderId), BlockType(_BlockType)
	{

	}
};

class LoadManager
{
public:
	~LoadManager(){}


	static LoadManager& Get()
	{
		static LoadManager LoadManager;
		return LoadManager;
	}

	LoadManager(const LoadManager& Others) = delete;
	LoadManager& operator=(const LoadManager&) = delete;

	void SaveMap(int BirdCount);
	bool LoadMap(int num);
	void ClearMap();
	ABlock *SpawnBlock(const FSpawnInfo& ObstacleInfo);

private:
	LoadManager() {}
	inline static int MapNumber = 0;
	void SetBlockImage(ABlock* Block, const FSpawnInfo& ObstacleInfo);
};