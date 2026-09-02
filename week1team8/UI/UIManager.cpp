#include "UIManager.h"
#include "../GameManager.h"
#include "../LoadManager.h"
#include <cmath>


UIManager::~UIManager()
{
	Release();
}

ID2D1Bitmap* UIManager::LoadBitmapFromFile(const wchar_t* uri)
{
	if (!uri) return nullptr;
	return URenderer::GetInstance().LoadBitmapFromFile(uri);
}

bool UIManager::Initialize(int nWidth, int nHeight)
{
	URenderer& renderer = URenderer::GetInstance();
	D2DRenderTarget = renderer.D2DRenderTarget;
	DWriteFactory = renderer.DWriteFactory;
	if (!D2DRenderTarget || !DWriteFactory) return false;

	HRESULT hr;
	hr = D2DRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &Brush);
	if (FAILED(hr)) return false;

	hr = DWriteFactory->CreateTextFormat(
		L"Malgun Gothic",
		nullptr,
		DWRITE_FONT_WEIGHT_EXTRA_BOLD,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		32.0f,
		L"ko-kr",
		&FloatingFont
	);
	if (FAILED(hr)) return false;

	screenWidth = nWidth;
	screenHeight = nHeight;

	//시작페이지
	{

		UUIPage* startPage = new UUIPage(EPageType::Starting);
		
		//배경
		UUIBackground* startBg = new UUIBackground(
			L"Assets/img/startimg.png",
			screenWidth * 0.5f,
			screenHeight * 0.5f,
			static_cast<float>(screenWidth),
			static_cast<float>(screenHeight)
		);
		startPage->AddChild(startBg);

		//게임 시작 버튼
		UUIButton* startbtn = new UUIButton(
			L"Assets/img/gamestartbutton.png",
			screenWidth * 0.5f,
			screenHeight * 0.45f,
			500,
			200
		);
		startbtn->SetOnClick([this]() {
			ResetScore();
			ChangePage(EPageType::InGame);
			GameManager::GetInstance().Restart();
		});
		startPage->AddChild(startbtn);

		//게임 종료 버튼
		UUIButton* finishbtn = new UUIButton(
			L"Assets/img/gameoverbutton.png",
			screenWidth * 0.5f,
			screenHeight * 0.63f,
			500,
			200
		);
		finishbtn->SetOnClick([]() {
			PostQuitMessage(0);
		});
		startPage->AddChild(finishbtn);

		Pages[EPageType::Starting] = startPage;
	}

	//인게임페이지
	{
		UUIIngamePage* inGamePage = new UUIIngamePage();
		ID2D1Bitmap* pauseBtnBmp = renderer.LoadBitmapFromFile(L"Assets/img/pausebtn.png");
		inGamePage->Initialize(DWriteFactory, D2DRenderTarget, pauseBtnBmp, nullptr, screenWidth, screenHeight);
		if (inGamePage->PauseBtn)
		{
			inGamePage->PauseBtn->SetOnClick([this]() {
				ChangePage(EPageType::Pause);
				GameManager::GetInstance().Pause();
			});
		}
		Pages[EPageType::InGame] = inGamePage;
	}

	//일시정지페이지
	{
		UUIPage* pausePage = new UUIPage(EPageType::Pause);

		//반투명 검은색 배경 생성
		UUIBackground* pauseBg = new UUIBackground(
			0.6f,
			screenWidth * 0.5f,
			screenHeight * 0.5f,
			static_cast<float>(screenWidth),
			static_cast<float>(screenHeight)
		);
		pausePage->AddChild(pauseBg);

		//일시정지 팝업
		UUIBackground* popupBoard = new UUIBackground(
			L"Assets/img/pauseimg.png",
			screenWidth * 0.5f,
			screenHeight * 0.48f,
			850.0f,
			750.0f
		);
		pausePage->AddChild(popupBoard);

		//계속하기 버튼
		UUIButton* continueBtn = new UUIButton(
			L"Assets/img/continuebtn.png",
			screenWidth * 0.5f,
			screenHeight * 0.45f,
			300.0f,
			110.0f
		);
		continueBtn->SetOnClick([this]() {
			ChangePage(EPageType::InGame);
			GameManager::GetInstance().Resume();
		});
		pausePage->AddChild(continueBtn);

		//재시작 버튼
		UUIButton* retryBtn = new UUIButton(
			L"Assets/img/retrybtn.png",
			screenWidth * 0.5f,
			screenHeight * 0.55f,
			300.0f,
			100.0f
		);

		retryBtn->SetOnClick([this]() {
			ResetScore();
			LoadManager::Get().LoadMap(GameManager::GetInstance().GetCurlvl());
			ChangePage(EPageType::InGame);
			GameManager::GetInstance().Restart();
		});
		pausePage->AddChild(retryBtn);

		//게임 종료 버튼
		UUIButton* finishbtn = new UUIButton(
			L"Assets/img/gameoverbutton.png",
			screenWidth * 0.5f,
			screenHeight * 0.65f,
			300.0f,
			100.0f
		);
		finishbtn->SetOnClick([]() {
			PostQuitMessage(0);
			});

		pausePage->AddChild(finishbtn);



		Pages[EPageType::Pause] = pausePage;
	}

	//엔딩페이지
	{
		UUIPage* endPage = new UUIPage(EPageType::Ending);

		UUIButton* Resultbtn = new UUIButton(
			L"Assets/img/GameClear.png",
			screenWidth * 0.5f, screenHeight * 0.5f, 450, 850,
			true,
			1200.0f
		);

		Resultbtn->SetOnClick([this]() {
			GameManager::GetInstance().Menu();
		});

		endPage->AddChild(Resultbtn);

		Pages[EPageType::Ending] = endPage;
	}

	ChangePage(EPageType::Starting);

	return true;
}

void UIManager::AddScore(int points)
{
	if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame)))
	{
		inGame->AddScore(points);
	}
}


void UIManager::ResetScore()
{
	if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame)))
	{
		inGame->ResetScore();
	}
}

void UIManager::SpawnFloatingText(float score, float screenX, float screenY, D2D1_COLOR_F color)
{
	if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame)))
	{
		inGame->SpawnFloatingText(score, screenX, screenY, color);
	}
}

void UIManager::Update(float deltaTime)
{
	if (CurrentPage)
	{
		CurrentPage->Update(deltaTime);
	}
}

void UIManager::Render(int birdsLeft)
{
	if (!D2DRenderTarget)
		return;

	D2DRenderTarget->BeginDraw();

	if (CurrentPage)
	{
		if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(CurrentPage))
		{
			inGame->SetBirdsLeft(birdsLeft);
		}
		CurrentPage->Render(D2DRenderTarget, Brush, FloatingFont);
	}

	D2DRenderTarget->EndDraw();
}

void UIManager::Release()
{
	if (FloatingFont) { FloatingFont->Release(); FloatingFont = nullptr; }
	if (Brush) { Brush->Release(); Brush = nullptr; }
	D2DRenderTarget = nullptr;
	DWriteFactory = nullptr;

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
		score = 1000.0f;
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
	else if (colAId == EColliderId::BLOCK && colBId == EColliderId::BLOCK)
	{
		UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame));
		if (inGame)
		{
			UUIFloatingText* ft = nullptr;
			float maxMergeDistance = 100.0f;
			float minDistanceSq = maxMergeDistance * maxMergeDistance;

			for (auto& it : inGame->FloatingTexts)
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
	for (const auto& it : infos)
	{
		pair<float, float> pos = WorldToScreen(it.contactPoint);
		CalPos(it.colAId, it.colBId, pos.first, pos.second);
	}
}

void UIManager::ChangePage(EPageType newPageType)
{
	if (CurrentPage)
	{
		CurrentPage->Hide();
	}

	auto it = Pages.find(newPageType);
	if (it != Pages.end())
	{
		CurrentPage = it->second;
		CurrentPage->Show();
	}

	switch (newPageType)
	{
	case EPageType::Starting:
		ResetScore();
		break;
	case EPageType::InGame:
		GameManager::GetInstance().SetGameState(GameState::Play);
		break;
	case EPageType::Pause:
		GameManager::GetInstance().SetGameState(GameState::Pause);
		break;
	default:
		break;
	}





}



void UIManager::GotoEnding(GameState gs)
{
	if (CurrentPage)
	{
		CurrentPage->Hide();
	}

	auto it = Pages.find(EPageType::Ending);
	if (it != Pages.end())
	{
		UUIIngamePage* igpg = dynamic_cast<UUIIngamePage*>(CurrentPage);
		if (igpg)
		{
			igpg->TargetScore = 0;
			igpg->DisplayScore = 0;
		}

		CurrentPage = it->second;

		switch (gs)
		{
		case GameState::GameOver:
			CurrentPage->Show(false);
			break;
		case GameState::GameClear:
			CurrentPage->Show(true);
			break;
		default:
			break;
		}
		
	}
}

void UIManager::LevelChanged(int curlevel)
{
	UUIIngamePage* igpage = dynamic_cast<UUIIngamePage*>(CurrentPage);
	if (igpage)
	{
		igpage->ClearFlowtingText();
	}
}

void UIManager::DrawBirdPath(const std::vector<FVector>& vertices)
{
	if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame)))
	{
		inGame->SetTrajectoryPoints(vertices);
	}
}

void UIManager::ClearBirdPath()
{
	if (UUIIngamePage* inGame = dynamic_cast<UUIIngamePage*>(GetPage(EPageType::InGame)))
	{
		inGame->ClearTrajectoryPoints();
	}
}



std::pair<float, float> UIManager::WorldToScreen(const FVector& worldPos) 
{
	float aspect = (screenHeight > 0) ? (static_cast<float>(screenWidth) / static_cast<float>(screenHeight)) : (16.0f / 9.0f);
	float screenX = (worldPos.x / aspect + 1.0f) * 0.5f * static_cast<float>(screenWidth);
	float screenY = (1.0f - worldPos.y) * 0.5f * static_cast<float>(screenHeight);

	return { screenX, screenY };
}
