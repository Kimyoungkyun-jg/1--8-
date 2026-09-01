#include "UIManager.h"
#include <cmath>

UIManager::~UIManager()
{
	Release();
}

bool UIManager::Initialize(IDXGISwapChain* swapChain, int nWidth,int nHeight)
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

	//고정 HUD 폰트
	hr = DWriteFactory->CreateTextFormat(
		L"맑은 고딕",
		nullptr,
		DWRITE_FONT_WEIGHT_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		26.0f,
		L"ko-kr",
		&HUDFont
	);
	if (FAILED(hr)) return false;

	//화면 점수 폰트
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


	//WIC Factory 초기화
	CoInitialize(nullptr); // COM 초기화
	CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&WICFactory)
	);

	if (FAILED(hr))
	{
		return false;
	}

	//비트맵 로드
	const wchar_t* imagePath = L"C:\\Users\\JUNGLE\\Desktop\\1--8-\\week1team8\\Assets\\img\\pausebtn.png";
	PauseButtonBitmap = LoadBitmapFromFile(imagePath);

	screenWidth = nWidth;
	screenHeight = nHeight;

	return true;
}

void UIManager::AddScore(int points)
{
	TargetScore += points;
}

void UIManager::SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color)
{
	FloatingTexts.emplace_back(score, screenX, screenY, color);
}

void UIManager::SpawnFloatingText(const wchar_t* text, float screenX, float screenY, D2D1_COLOR_F color)
{
	FloatingTexts.emplace_back(text, screenX, screenY, color);
}

void UIManager::Update(float deltaTime)
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

	for (auto it = FloatingTexts.begin(); it != FloatingTexts.end(); )
	{
		it->Update(deltaTime);

		if (it->bIsFinished) 
		{
			if (it->TargetScore > 0.0f)
			{
				AddScore(static_cast<int>(it->TargetScore)); 
			}

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
	// 좌상단 고정 점수표
	if (!D2DRenderTarget)
		return;

	D2DRenderTarget->BeginDraw();

	wchar_t scoreText[128];
	swprintf_s(
		scoreText,
		L"SCORE: %06d   BIRDS: %d",
		static_cast<int>(DisplayScore),
		birdsLeft
	);

	Brush->SetColor(D2D1::ColorF(D2D1::ColorF::White));
	D2D1_RECT_F hudRect = D2D1::RectF(TargetHUD_X, TargetHUD_Y, 600.0f, 80.0f);

	D2DRenderTarget->DrawText(
		scoreText,
		static_cast<UINT32>(wcslen(scoreText)),
		HUDFont,
		&hudRect,
		Brush,
		D2D1_DRAW_TEXT_OPTIONS_NONE,
		DWRITE_MEASURING_MODE_NATURAL
	);

	// 화면에 뜨는 점수 
	for (size_t i = 0; i < FloatingTexts.size(); ++i)
	{
		const FFloatingText& ft = FloatingTexts[i];

		Brush->SetColor(ft.Color);

		float width = 120.0f;  // 점수 텍스트 길이에 맞춰 적절한 폭으로 설정
		float height = 40.0f;  // 양수로 설정

		D2D1_RECT_F rect = D2D1::RectF(
			ft.X,
			ft.Y,
			ft.X + width,
			ft.Y + height
		);

		// 실제 rect의 정중앙 좌표 계산
		D2D1_POINT_2F center = D2D1::Point2F(ft.X + width * 0.5f, ft.Y + height * 0.5f);

		D2D1::Matrix3x2F scaleMatrix = D2D1::Matrix3x2F::Scale(
			D2D1::SizeF(ft.ftscale, ft.ftscale),
			center
		);

		D2DRenderTarget->SetTransform(scaleMatrix);

		wchar_t dynamicScoreStr[32];
		swprintf_s(dynamicScoreStr, L"%d", static_cast<int>(ft.CurrentScore));

		D2DRenderTarget->DrawText(
			dynamicScoreStr,
			static_cast<UINT32>(wcslen(dynamicScoreStr)),
			FloatingFont,
			&rect,
			Brush,
			D2D1_DRAW_TEXT_OPTIONS_NONE,
			DWRITE_MEASURING_MODE_NATURAL
		);

		// 변환 행렬 초기화
		D2DRenderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
	}


	//일시 정지 버튼

	if (PauseButtonBitmap)
	{
		D2DRenderTarget->DrawBitmap(
			PauseButtonBitmap,
			&PauseButtonRect,
			1.0f, // 불투명도 (Opacity)
			D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
		);
	}


	D2DRenderTarget->EndDraw();
}

void UIManager::Release()
{
	if (PauseButtonBitmap) { PauseButtonBitmap->Release(); PauseButtonBitmap = nullptr; }
	if (WICFactory) { WICFactory->Release(); WICFactory = nullptr; }

	if (FloatingFont) { FloatingFont->Release(); FloatingFont = nullptr; }
	if (HUDFont) { HUDFont->Release(); HUDFont = nullptr; }
	if (Brush) { Brush->Release(); Brush = nullptr; }
	if (D2DRenderTarget) { D2DRenderTarget->Release(); D2DRenderTarget = nullptr; }
	if (DWriteFactory) { DWriteFactory->Release(); DWriteFactory = nullptr; }
	if (D2DFactory) { D2DFactory->Release(); D2DFactory = nullptr; }

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
		FFloatingText* ft = nullptr;
		float maxMergeDistance = 100.0f;
		float minDistanceSq = maxMergeDistance * maxMergeDistance;

		for (auto& it : FloatingTexts)
		{
			if (it.bIsFlyingToHUD || it.bIsFinished) continue;
			float dx = it.X - colposx;
			float dy = it.Y - colposy;
			float distSq = dx * dx + dy * dy; // 거리의 제곱

			if (distSq < minDistanceSq) // 가장 가까운 대상 탐색
			{
				minDistanceSq = distSq;
				ft = &it;
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

	// 이미지 파일 디코더 생성
	if (FAILED(WICFactory->CreateDecoderFromFilename(uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)))
	{
		return nullptr;
	}

	// 첫 번째 프레임 디코딩
	if (FAILED(decoder->GetFrame(0, &frame)))
	{
		decoder->Release();
		return nullptr;
	}

	// Direct2D 호환 32bppPBGRA 포맷 변환기 생성
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

	// D2D 비트맵 생성
	D2DRenderTarget->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap);

	// 임시 WIC 객체 해제
	converter->Release();
	frame->Release();
	decoder->Release();

	return bitmap;
}