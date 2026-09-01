#pragma once

#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")

class SoundManager
{
public:
	static SoundManager& GetInstance()
	{
		static SoundManager instance;
		return instance;
	}

	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

	bool Initialize()
	{
		HRESULT hr;

		// XAudio 생성
		hr = XAudio2Create(&m_XAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);

		if (FAILED(hr))
		{
			return false;
		}

		// Master Voice 생성
		hr = m_XAudio2->CreateMasteringVoice(&m_MasterVoice);

		if (FAILED(hr))
		{
			m_XAudio2->Release();
			m_XAudio2 = nullptr;

			return false;
		}

		return true;
	}
	void Shutdown()
	{
		if (m_MasterVoice)
		{
			m_MasterVoice->DestroyVoice();
			m_MasterVoice = nullptr;
		}

		if (m_XAudio2)
		{
			m_XAudio2->Release();
			m_XAudio2 = nullptr;
		}
	}

	SoundManager() = default;
	~SoundManager() = default;

	IXAudio2* m_XAudio2 = nullptr;
	IXAudio2MasteringVoice* m_MasterVoice = nullptr;
};

