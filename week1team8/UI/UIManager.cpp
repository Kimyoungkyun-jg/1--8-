#include "UIManager.h"
#include <cmath>

UIManager::~UIManager()
{
	Release();
}

bool UIManager::Initialize(IDXGISwapChain* swapChain, int nWidth, int nHeight)
{
	if (!swapChain) return false;

	HRESULT hr;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
	if (FAILED(hr)) return false;

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&DWriteFactory));
	if (FAILED(hr)) return false;

	IDXGISurface* surface = nullptr;
	hr = swapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
	if (FAILED(hr)) return false;

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	hr = D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &D2DRenderTarget);
	surface->Release();
	if (FAILED(hr)) return false;

	hr = D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &Brush);
	if (FAILED(hr)) return false;

	hr = DWriteFactory->CreateTextFormat(
		L"맑은 고딕",
		nullptr,
		DWRITE_FONT_WEIGHT_EXTRA_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		32.0f,
		L"ko-kr",
		&FloatingFont
	);
	if (FAILED(hr)) return false;

	CoInitialize(nullptr);
	CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&WICFactory)
	);

	screenWidth = nWidth;
	screenHeight = nHeight;

	for (int i = 0; i < 4; i++)
	{
		EPageType pageType = static_cast<EPageType>(i);
		UUIPage* page = new UUIPage(pageType);

		switch (pageType)
		{
		case EPageType::Starting:
			break;

		case EPageType::InGame:
		{
			// 1. 좌상단 HUD
			MainHUD = new UUIHUDText(TargetHUD_X, TargetHUD_Y);
			MainHUD->Initialize(DWriteFactory, D2DRenderTarget);
			page->AddChild(MainHUD);

			// 2. 우상단 일시정지 버튼
			UUIButton* pauseBtn = new UUIButton();
			const wchar_t* imagePath = L"C:\\Users\\JUNGLE\\Desktop\\1--8-\\week1team8\\Assets\\img\\pausebtn.png";
			pauseBtn->SetImagePath(imagePath);
			pauseBtn->ButtonBitmap = LoadBitmapFromFile(imagePath);
			pauseBtn->SetPoisition(800.0f, 30.0f, 50.0f, 50.0f);
			page->AddChild(pauseBtn);
			break;
		}

		case EPageType::Pause:
			break;

		case EPageType::Ending:
			break;
		}

		Pages[pageType] = page;
	}

	ChangePage(EPageType::InGame);

	return true;
}

void UIManager::AddScore(int points)
{
	TargetScore += points;
}

void UIManager::SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color)
{
	UUIFloatingText* newText = new UUIFloatingText(score, screenX, screenY, color);
	FloatingTexts.push_back(newText);

	UUIPage* inGamePage = GetPage(EPageType::InGame);
	if (inGamePage)
	{
		inGamePage->AddChild(newText);
	}
}

void UIManager::Update(float deltaTime)
{
	if (!CurrentPage)
		return;


	
	switch (CurrentPage->PageType)
	{
	case EPageType::Starting:
		break;
	case EPageType::InGame:
		UpdateScore(deltaTime);
		UpdateFloatingTexts(deltaTime);
		CurrentPage->Update(deltaTime);

		break;
	case EPageType::Pause:
		break;
	case EPageType::Ending:
		break;
	default:
		break;
	}
	


}

void UIManager::UpdateScore(float deltaTime)
{
	if (DisplayScore < TargetScore) 
	{
		float speed = 10.0f;
		DisplayScore += (TargetScore - DisplayScore) * (speed * deltaTime);
		if (std::abs(TargetScore - DisplayScore) < 1.0f)
		{
			DisplayScore = static_cast<float>(TargetScore);
		}
	}
}

void UIManager::UpdateFloatingTexts(float deltaTime)
{
	UUIPage* inGamePage = GetPage(EPageType::InGame);

	for (auto it = FloatingTexts.begin(); it != FloatingTexts.end(); )
	{
		UUIFloatingText* ft = *it;
		ft->Update(deltaTime);

		if (ft->bIsFinished)
		{
			if (ft->TargetScore > 0.0f)
			{
				AddScore(static_cast<int>(ft->TargetScore));
			}

			if (inGamePage)
			{
				auto& children = inGamePage->ChildUIObjects;
				auto cIt = std::find(children.begin(), children.end(), ft);
				if (cIt != children.end())
				{
					children.erase(cIt);
				}
			}

			delete ft;
			it = FloatingTexts.erase(it);
		}
		else
		{
			++it;
		}
	}
}

void UIManager::Render(int birdsLeft)
{
	if (!D2DRenderTarget)
		return;

	D2DRenderTarget->BeginDraw();

	if (MainHUD)
	{
		MainHUD->SetData(DisplayScore, birdsLeft);
	}

	if (CurrentPage)
	{
		CurrentPage->Render(D2DRenderTarget, Brush, FloatingFont);
	}


	D2DRenderTarget->EndDraw();
}

void UIManager::Release()
{
	if (WICFactory) { WICFactory->Release(); WICFactory = nullptr; }

	if (FloatingFont) { FloatingFont->Release(); FloatingFont = nullptr; }
	if (Brush) { Brush->Release(); Brush = nullptr; }
	if (D2DRenderTarget) { D2DRenderTarget->Release(); D2DRenderTarget = nullptr; }
	if (DWriteFactory) { DWriteFactory->Release(); DWriteFactory = nullptr; }
	if (D2DFactory) { D2DFactory->Release(); D2DFactory = nullptr; }

	for (auto& pair : Pages)
	{
		delete pair.second;
	}
	Pages.clear();
	CurrentPage = nullptr;
}

void UIManager::CalPos(EColliderId colAId, EColliderId colBId, float colposx, float colposy)
{
	float score = 0.0f;
	D2D1_COLOR_F color = D2D1::ColorF(D2D1::ColorF::Gold);

	if ((colAId == EColliderId::BIRD && colBId == EColliderId::PIG) ||
		(colAId == EColliderId::PIG  && colBId == EColliderId::BIRD))
	{
		score += 1000.0f;
		color = D2D1::ColorF(D2D1::ColorF::Gold);
	}
	else if ((colAId == EColliderId::BIRD  && colBId == EColliderId::BLOCK) ||
		     (colAId == EColliderId::BLOCK && colBId == EColliderId::BIRD))
	{
		score = 700.0f;
		color = D2D1::ColorF(D2D1::ColorF::Yellow);
	}
	else if ((colAId == EColliderId::BLOCK && colBId == EColliderId::PIG) ||
		     (colAId == EColliderId::PIG   && colBId == EColliderId::BLOCK))
	{
		score = 2000.0f;
		color = D2D1::ColorF(D2D1::ColorF::LightGreen);
	}
	else if(colAId == EColliderId::BLOCK && colBId == EColliderId::BLOCK)
	{
		UUIFloatingText* ft = nullptr;
		float maxMergeDistance = 100.0f;
		float minDistanceSq = maxMergeDistance * maxMergeDistance;

		for (auto& it : FloatingTexts)
		{
			if (it->bIsFlyingToHUD || it->bIsFinished) continue;
			float dx = it->X - colposx;
			float dy = it->Y - colposy;
			float distSq = dx * dx + dy * dy;

			if (distSq < minDistanceSq)
			{
				minDistanceSq = distSq;
				ft = it;
			}
		}

		if (ft)
		{
			ft->TargetScore += 500;
			ft->FloatTimer += 0.4f;
			ft->ftscale = 1.5f;
		}
		
		return;
	}
	else
	{
		return;
	}

	SpawnFloatingText(score, colposx, colposy, color);
}

void UIManager::GetCollisionInfos(std::vector<CollisionInfo> infos) 
{
	if (infos.size() > 0)
	{
		for (auto& it : infos)
		{
			pair<float, float> pos = WorldToScreen(it.contactPoint);
			CalPos(it.colAId, it.colBId, pos.first, pos.second);
		}
	}
}

std::pair<float, float> UIManager::WorldToScreen(const FVector& worldPos) 
{
	float screenX = (worldPos.x + 1.0f) * 0.5f * screenWidth;
	float screenY = (1.0f - worldPos.y) * 0.5f * screenHeight;

	return { screenX, screenY };
}

ID2D1Bitmap* UIManager::LoadBitmapFromFile(const wchar_t* uri) 
{
	if (!WICFactory || !D2DRenderTarget) return nullptr;

	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;
	ID2D1Bitmap* bitmap = nullptr;

	if (FAILED(WICFactory->CreateDecoderFromFilename(uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)))
	{
		return nullptr;
	}

	if (FAILED(decoder->GetFrame(0, &frame)))
	{
		decoder->Release();
		return nullptr;
	}

	if (FAILED(WICFactory->CreateFormatConverter(&converter)))
	{
		frame->Release();
		decoder->Release();
		return nullptr;
	}

	converter->Initialize(
		frame,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	D2DRenderTarget->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap);

	converter->Release();
	frame->Release();
	decoder->Release();

	return bitmap;
}
