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

	void SaveMap(const ABird* Bird, const ASlingShot* SlingShot);

private:
	LoadManager() {}
	inline static int MapNumber = 0;
};