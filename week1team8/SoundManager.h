#pragma once
#include <xaudio2.h>
#include <vector>
#include <string>
#include <unordered_map>

// 효과음 재생이 끝나면 자동으로 Voice를 파괴해주는 콜백
class VoiceCallback : public IXAudio2VoiceCallback
{
public:
	void OnStreamEnd() noexcept override {}
	void OnVoiceProcessingPassEnd() noexcept override {}
	void OnVoiceProcessingPassStart(UINT32) noexcept override {}
	void OnBufferStart(void*) noexcept override {}
	void OnLoopEnd(void*) noexcept override {}
	void OnVoiceError(void*, HRESULT) noexcept override {}

	void OnBufferEnd(void* pBufferContext) noexcept override
	{
		// PlaySFX에서 pContext에 Voice 포인터를 넣어뒀다가 여기서 파괴
		IXAudio2SourceVoice* pVoice = reinterpret_cast<IXAudio2SourceVoice*>(pBufferContext);
		if (pVoice) pVoice->DestroyVoice();
	}
};

struct SoundData
{
	WAVEFORMATEX wfx = {};
	std::vector<BYTE> audioBytes;
};

class SoundManager
{
public:
	// 싱글톤 접근점
	static SoundManager& Get()
	{
		static SoundManager instance;
		return instance;
	}

	// 복사/이동 금지
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;
	SoundManager(SoundManager&&) = delete;
	SoundManager& operator=(SoundManager&&) = delete;

	bool Initialize()
	{
		if (bInitialized) return true;

		if (FAILED(CoInitializeEx(nullptr, COINIT_MULTITHREADED)))
			return false;

		if (FAILED(XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR)))
			return false;

		if (FAILED(pXAudio2->CreateMasteringVoice(&pMasterVoice)))
			return false;

		bInitialized = true;
		return true;
	}

	void Shutdown()
	{
		if (!bInitialized) return;

		if (pBgmVoice) { pBgmVoice->Stop(); pBgmVoice->DestroyVoice(); pBgmVoice = nullptr; }
		if (pMasterVoice) { pMasterVoice->DestroyVoice(); pMasterVoice = nullptr; }
		if (pXAudio2) { pXAudio2->Release(); pXAudio2 = nullptr; }

		CoUninitialize();
		bInitialized = false;
	}

	bool LoadSound(const std::string& key, const std::wstring& filePath);
	// WAV 파일을 읽어서 sounds[key]에 저장하는 함수 (WAV 파서 별도 구현 필요)

	void PlayBGM(const std::string& key, bool loop = true, float volume = 1.0f)
	{
		auto it = sounds.find(key);
		if (it == sounds.end()) return;

		if (pBgmVoice) { pBgmVoice->Stop(); pBgmVoice->DestroyVoice(); pBgmVoice = nullptr; }

		pXAudio2->CreateSourceVoice(&pBgmVoice, &it->second.wfx);

		XAUDIO2_BUFFER buffer = {};
		buffer.AudioBytes = static_cast<UINT32>(it->second.audioBytes.size());
		buffer.pAudioData = it->second.audioBytes.data();
		buffer.LoopCount = loop ? XAUDIO2_LOOP_INFINITE : 0;

		pBgmVoice->SetVolume(volume);
		pBgmVoice->SubmitSourceBuffer(&buffer);
		pBgmVoice->Start(0);
	}

	void StopBGM()
	{
		if (pBgmVoice) { pBgmVoice->Stop(); pBgmVoice->DestroyVoice(); pBgmVoice = nullptr; }
	}

	void PlaySFX(const std::string& key, float volume = 1.0f)
	{
		auto it = sounds.find(key);
		if (it == sounds.end()) return;

		IXAudio2SourceVoice* pVoice = nullptr;
		pXAudio2->CreateSourceVoice(&pVoice, &it->second.wfx, 0,
			XAUDIO2_DEFAULT_FREQ_RATIO, &voiceCallback);

		XAUDIO2_BUFFER buffer = {};
		buffer.AudioBytes = static_cast<UINT32>(it->second.audioBytes.size());
		buffer.pAudioData = it->second.audioBytes.data();
		buffer.pContext = pVoice; // 콜백에서 DestroyVoice 하기 위해 전달
		buffer.Flags = XAUDIO2_END_OF_STREAM;

		pVoice->SetVolume(volume);
		pVoice->SubmitSourceBuffer(&buffer);
		pVoice->Start(0);
	}

private:
	SoundManager() = default;
	~SoundManager() { Shutdown(); }

	IXAudio2* pXAudio2 = nullptr;
	IXAudio2MasteringVoice* pMasterVoice = nullptr;
	IXAudio2SourceVoice* pBgmVoice = nullptr;
	VoiceCallback voiceCallback;
	std::unordered_map<std::string, SoundData> sounds;
	bool bInitialized = false;
};
