#pragma once

class ABird;
class ASlingShot;

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

	void SaveMap();
	bool LoadMap(int num);
	void ClearMap();

private:
	LoadManager() {}
	inline static int MapNumber = 0;
};