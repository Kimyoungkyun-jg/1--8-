#include "SoundManager.h"
#include <fstream>

namespace
{
	// 4바이트 청크 ID 비교용
	bool MatchChunkID(const char* a, const char* b)
	{
		return a[0] == b[0] && a[1] == b[1] && a[2] == b[2] && a[3] == b[3];
	}
}

bool SoundManager::LoadSound(const std::string& key, const std::wstring& filePath)
{
	std::ifstream file(filePath, std::ios::binary);
	if (!file.is_open())
		return false;

	// --- RIFF 헤더 확인 ---
	char riffId[4];
	file.read(riffId, 4);
	if (!MatchChunkID(riffId, "RIFF"))
		return false;

	UINT32 riffSize = 0;
	file.read(reinterpret_cast<char*>(&riffSize), sizeof(riffSize));

	char waveId[4];
	file.read(waveId, 4);
	if (!MatchChunkID(waveId, "WAVE"))
		return false;

	// --- 청크 순회하며 fmt / data 찾기 ---
	WAVEFORMATEX wfx = {};
	std::vector<BYTE> audioData;
	bool foundFmt = false;
	bool foundData = false;

	while (file && !(foundFmt && foundData))
	{
		char chunkId[4];
		UINT32 chunkSize = 0;

		file.read(chunkId, 4);
		file.read(reinterpret_cast<char*>(&chunkSize), sizeof(chunkSize));
		if (!file) break;

		if (MatchChunkID(chunkId, "fmt "))
		{
			// fmt 청크는 최소 16바이트 (PCM 기준). 확장 포맷 대비해서 min으로 읽음
			UINT32 readSize = (std::min)(chunkSize, static_cast<UINT32>(sizeof(WAVEFORMATEX)));
			file.read(reinterpret_cast<char*>(&wfx), readSize);

			// 청크 크기가 구조체보다 크면 남은 바이트 건너뛰기 (WAVEFORMATEXTENSIBLE 등)
			if (chunkSize > readSize)
				file.seekg(chunkSize - readSize, std::ios::cur);

			foundFmt = true;
		}
		else if (MatchChunkID(chunkId, "data"))
		{
			audioData.resize(chunkSize);
			file.read(reinterpret_cast<char*>(audioData.data()), chunkSize);
			foundData = true;
		}
		else
		{
			// LIST, fact 등 관심 없는 청크는 건너뛰기
			file.seekg(chunkSize, std::ios::cur);
		}

		// 청크는 짝수 바이트 정렬 (홀수 크기면 패딩 1바이트)
		if (chunkSize % 2 != 0)
			file.seekg(1, std::ios::cur);
	}

	if (!foundFmt || !foundData)
		return false;

	SoundData soundData;
	soundData.wfx = wfx;
	soundData.audioBytes = std::move(audioData);

	sounds[key] = std::move(soundData);
	return true;
}
