#pragma comment(lib, "user32")				

#include <windows.h>
#include <windowsx.h>
#include <d3d11.h>						
#include <d3dcompiler.h>
#include <vector>
#include <cstdlib>

#include "ImGui/imgui.h"				
#include "ImGui/imgui_internal.h"		
#include "ImGui/imgui_impl_dx11.h"		
#include "ImGui/imgui_impl_win32.h"		

#include "Vector.h"
#include "Renderer.h"
#include "UObject.h"
#include "Global.h"
#include "TemplateLibrary.h"
#include "LoadManager.h"

//모든 매니저 헤더파일
#include "GameManager.h"
#include "UI/UIManager.h"
#include "UI/UUIBackground.h"
#include "CollisionManager.h"
#include "ObjectManager.h"
#include "SoundManager.h"
#include "Effects/EffectManager.h"

bool bUseGravity = true;

FVector ScreenToWorld(HWND hwnd, int MouseX, int MouseY)
{
	RECT rec;
	GetClientRect(hwnd, &rec);

	float width = (float)(rec.right - rec.left);
	float height = (float)(rec.bottom - rec.top);

	if (width <= 0.0f || height <= 0.0f)
	{
		return FVector(0.0f, 0.0f, 0.0f);
	}

	float aspect = width / height;


	float worldX = (2.0f * (float)MouseX / width - 1.0f) * aspect;
	float worldY = 1.0f - (2.0f * (float)MouseY / height);

	return FVector(worldX, worldY, 0.0f);
}

// ScreenToWorld의 역변환. 크기는 ImGui의 DisplaySize에서 가져와야 한다.
// GetClientRect의 물리 픽셀을 쓰면 DPI 배율만큼 밀린다.
ImVec2 WorldToScreen(const FVector& World)
{
	ImGuiIO& io = ImGui::GetIO();

	float width = io.DisplaySize.x;
	float height = io.DisplaySize.y;

	if (width <= 0.0f || height <= 0.0f)
	{
		return ImVec2(0.0f, 0.0f);
	}

	float aspect = width / height;

	float screenX = (World.x / aspect + 1.0f) * 0.5f * width;
	float screenY = (1.0f - World.y) * 0.5f * height;

	return ImVec2(screenX, screenY);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// ImGui 메시지 처리
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
	{
		return true;
	}

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	WCHAR WindowClass[] = L"JungleWindowClass";
	WCHAR Title[] = L"Game Tech Lab";

	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndclass);

	int windowWidth = 1920;
	int windowHeight = 1080; //해상도

	HWND hWnd = CreateWindowExW(0, WindowClass, Title,
		WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, windowWidth, windowHeight, nullptr, nullptr, hInstance, nullptr);

	// 렌더러 초기화
	URenderer& renderer = URenderer::GetInstance();
	renderer.Create(hWnd);
	renderer.CreateShader();
	renderer.CreateConstantBuffer();
	renderer.CreateVertexBufferInfos();

	// ImGui 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	// 프레임 관리
	const int targetFPS = 144;
	const double targetFrameTime = 1000.0 / targetFPS;	// 한 프레임의 목표 시간 (밀리초 단위)

	// 물리는 프레임과 분리해 항상 같은 크기로 진행한다.
	// 큰 걸음 한 번 대신 작은 걸음 여러 번이라 물체가 접촉을 뛰어넘지 않는다.
	const double fixedDeltaTime = 1.0 / 144.0;
	const double maxAccumulated = fixedDeltaTime * 5.0;
	double accumulator = 0.0;

	LARGE_INTEGER frequency;	// tick/sec
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0;
	float deltaTime = 0.0f;

	bool bIsExit = false;
	bool bPressed = false;
	int MouseX = 0, MouseY = 0;
	FVector WorldMouseXY;
	ACollider* PressedCollider = nullptr;

	float BlockWidth = 0.4;
	float BlockHeight = 0.05;
	float PigWidth = 0.15, PigHeight = 0.15;

	// 물리 디버그
	bool bPausePhysics = false;		// 켜면 물리가 멈춘다 (렌더와 UI는 계속 돈다)
	bool bStepOnce = false;			// Step 버튼이 눌린 프레임에만 한 번 진행
	bool bDrawContacts = true;		// 접촉점/법선 그리기
	bool bDrawColliders = true;		// 사각형 콜라이더의 OBB 외곽선 그리기
	float NormalLength = 40.0f;		// 법선 표시 길이 (픽셀)

	// 매니저 초기화
	UIManager& uiManager = UIManager::GetInstance();
	uiManager.Initialize(renderer, windowWidth, windowHeight);

	GameManager& gameManager = GameManager::GetInstance();
	gameManager.Initialize();

	UObjectManager& ObjectManager = UObjectManager::GetInstance();
	CollisionManager& CM = CollisionManager::GetInstance();
	LoadManager& LoadManager = LoadManager::Get();
	EffectManager& effectManager = EffectManager::GetInstance();
	effectManager.Initialize();

	SoundManager& SM = SoundManager::GetInstance();
	if (!SM.Initialize())
	{
		return 0;
	}

	SM.LoadSound("bgm_main", L"Assets/bgm_main.wav");
	SM.LoadSound("sfx_bird", L"Assets/sfx_bird.wav");
	SM.LoadSound("sfx_pig", L"Assets/sfx_pig.wav");
	SM.LoadSound("sfx_rock", L"Assets/sfx_rock.wav");

	SM.PlayBGM("bgm_main", true, 0.5f);

	ID2D1Bitmap* InGameBackgroundBitmap = renderer.LoadBitmapFromFile(L"Assets/img/ingamebackground.jpg");

	gameManager.Menu();

	// Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	while (bIsExit == false)
	{
		// 한 프레임 동작 (게임 매니저는 1 ~ 2를 관리함)
		// 0. 프레임 시작 기록 
		// 1. 입력 처리 (GameState 구분)
		// 2. 게임 루프 (GameState == Play) (이동, 충돌처리, 등)
		// 3. 렌더 준비 및 렌더 실행 (게임 -> UI -> ImGui 순)
		// 4. 프레임 교체 및 대기

		QueryPerformanceCounter(&startTime);

		// 입력 처리
		MSG msg;

		while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

			if (msg.message == WM_QUIT)
			{
				bIsExit = true;
				break;
			}
			else if (msg.message == WM_LBUTTONDOWN)
			{
				bPressed = true;
				Global::bIsLButtonPressed = true;
				MouseX = GET_X_LPARAM(msg.lParam);
				MouseY = GET_Y_LPARAM(msg.lParam);
				Global::MouseScreenX = static_cast<float>(MouseX); //글로벌에 다가 마우스 좌표 넘김
				Global::MouseScreenY = static_cast<float>(MouseY);
				WorldMouseXY = ScreenToWorld(hWnd, MouseX, MouseY);
				Global::MouseWorldPos = WorldMouseXY;
				SetCapture(msg.hwnd);

				bool bFound = false;
				if (gameManager.GetGameState() == GameState::Play)
				{
					for (ACollider* Collider : CM.colliders)
					{
						if (!Collider->bEditing) continue;
						FVector ColLoc = Collider->GetLocation();
						EPrimitive Primitive = Collider->GetPrimitive();
						if (Primitive == EPrimitive::Circle)
						{
							float dist = (ColLoc - WorldMouseXY).Length();
							if (dist <= Collider->GetScale().x / 2.f)
							{
								PressedCollider = Collider;
								Collider->Clicked();
								bFound = true;
								break;
							}
						}
						else if (Primitive == EPrimitive::Rectangle)
						{
							float halfx = Collider->GetScale().x / 2.f;
							float halfy = Collider->GetScale().y / 2.f;
							if (WorldMouseXY.x >= ColLoc.x - halfx
								&& WorldMouseXY.x <= ColLoc.x + halfx
								&& WorldMouseXY.y >= ColLoc.y - halfy
								&& WorldMouseXY.y <= ColLoc.y + halfy
								)
							{
								PressedCollider = Collider;
								Collider->Clicked();
								bFound = true;
								break;
							}
						}
					}
				}

				if (!bFound)
				{
					PressedCollider = nullptr;
				}
			}
			else if (msg.message == WM_LBUTTONUP)
			{
				bPressed = false;
				Global::bIsLButtonPressed = false;
				MouseX = GET_X_LPARAM(msg.lParam);
				MouseY = GET_Y_LPARAM(msg.lParam);
				Global::MouseScreenX = static_cast<float>(MouseX);
				Global::MouseScreenY = static_cast<float>(MouseY);
				WorldMouseXY = ScreenToWorld(hWnd, MouseX, MouseY);
				Global::MouseWorldPos = WorldMouseXY;

				if (PressedCollider)
				{
					if (PressedCollider->GetColliderId() == EColliderId::BIRD)
					{
						gameManager.GetSlingShot()->Released(WorldMouseXY);
					}
					else PressedCollider->Released(WorldMouseXY);
				}

				ReleaseCapture();
				SM.PlaySFX("sfx_bird");
			}
			else if (msg.message == WM_MOUSEMOVE)
			{
				MouseX = GET_X_LPARAM(msg.lParam);
				MouseY = GET_Y_LPARAM(msg.lParam);
				Global::MouseScreenX = static_cast<float>(MouseX);
				Global::MouseScreenY = static_cast<float>(MouseY);
				WorldMouseXY = ScreenToWorld(hWnd, MouseX, MouseY);
				Global::MouseWorldPos = WorldMouseXY;

				if (bPressed && PressedCollider)
				{
					PressedCollider->Pressed(WorldMouseXY);
				}
			}
		}

		// 일시정지 중에는 Step을 누른 프레임에만 한 번 진행. 렌더와 ImGui는 계속 돈다
		bool bAdvancePhysics = !bPausePhysics || bStepOnce;
		bStepOnce = false;

		deltaTime = static_cast<float>(elapsedTime * 0.001);

		uiManager.Update(deltaTime);
		effectManager.Update(deltaTime);

		// 물리 한 스텝. 항상 fixedDeltaTime만큼만 진행한다.
		auto StepPhysics = [&]()
			{
				for (ACollider* Collider : CM.colliders)
				{
					Collider->Move((float)fixedDeltaTime);
				}

				// 충돌 검사
				uiManager.GetCollisionInfos(CM.CheckCollisionAll((float)fixedDeltaTime));
			};

		// 흐른 시간을 쌓아두고 고정 크기로 꺼내 쓴다. 남은 건 다음 프레임으로 넘어간다.
		if (bPausePhysics)
		{
			// 멈춘 동안 시간이 쌓이면 풀었을 때 한꺼번에 몰아서 돈다
			accumulator = 0.0;

			if (bStepOnce)
			{
				StepPhysics();   // Step은 정확히 한 스텝
			}
		}
		else
		{
			accumulator += deltaTime;

			while (accumulator >= fixedDeltaTime)
			{
				StepPhysics();
				accumulator -= fixedDeltaTime;
			}
		}

		bStepOnce = false;

		//

		//매 프레임 UObject에 Tick 호출
		for (int i = ObjectManager.AllObjects.size() - 1; i >= 0; --i)
		{
			ObjectManager.AllObjects[i]->Tick(deltaTime);
		}

		// 렌더 준비
		renderer.Prepare();

		// 배경화면 그리기 (모든 게임 객체 뒤에 먼저 렌더링)
		if (InGameBackgroundBitmap)
		{
			renderer.DrawBitmap(InGameBackgroundBitmap, 0.0f, 0.0f, (float)windowWidth, (float)windowHeight);
		}

		renderer.PrepareShader();

		//이펙트 그리기
		effectManager.Render(renderer);


		// 그리기
		for (int i = 0; i < ObjectManager.AllObjects.size(); i++)
		{
			if (ObjectManager.AllObjects.empty()) break; //allobject 암것도 없으면 안그림

			if (AActor* Actor = dynamic_cast<AActor*>(ObjectManager.AllObjects[i]))
			{
				Actor->Draw(renderer);
			}
		}

		string s = "NONE";
		if (PressedCollider)
		{
			switch (PressedCollider->GetColliderId())
			{
			case EColliderId::BIRD:
				s = "BIRD";
				break;
			case EColliderId::PIG:
				s = "PIG";
				break;
			case EColliderId::BLOCK:
				s = "BLOCK";
				break;
			case EColliderId::NONE:
				s = "NONE";
				break;
			default:
				s = "NONE";
				break;
			}
		}

		// UI 그리기
		uiManager.Render(gameManager.GetBirdCount()+1);

		// ImGui
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 사각형 콜라이더의 OBB 외곽선. 스프라이트와 어긋나면 물리도 같이 어긋난 것
		if (bDrawColliders)
		{
			ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

			for (ACollider* Collider : CM.colliders)
			{
				if (Collider->GetPrimitive() != EPrimitive::Rectangle)
				{
					continue;
				}

				OBB Box = MakeOBB(Collider);

				// 잠든 물체는 회색 — 무리가 어떻게 잠드는지 눈으로 보려는 것
				ImU32 Color = Collider->IsSleeping()
					? IM_COL32(150, 150, 150, 255)
					: IM_COL32(80, 200, 255, 255);

				for (int i = 0; i < 4; i++)
				{
					DrawList->AddLine(WorldToScreen(Box.vertex[i]),
						WorldToScreen(Box.vertex[(i + 1) % 4]), Color, 2.0f);
				}

				// 꼭짓점 0. 블록을 돌리면 이 점도 같이 돌아야 한다
				DrawList->AddCircleFilled(WorldToScreen(Box.vertex[0]), 4.0f, Color);
			}
		}

		// 접촉점과 법선. 배경 draw list라 게임 화면 위, ImGui 창 아래에 그려진다
		if (bDrawContacts)
		{
			ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

			for (const CollisionInfo& Contact : CM.debugContacts)
			{
				for (int i = 0; i < Contact.pointCount; i++)
				{
					ImVec2 Point = WorldToScreen(Contact.points[i].position);

					// 법선은 방향 벡터라 y 부호만 뒤집으면 된다 (화면 y는 아래로 증가)
					ImVec2 Tip = ImVec2(Point.x + Contact.normal.x * NormalLength,
						Point.y - Contact.normal.y * NormalLength);

					// B -> A 방향, 즉 A를 밀어내는 쪽을 가리켜야 한다
					DrawList->AddLine(Point, Tip, IM_COL32(255, 64, 64, 255), 2.0f);
					DrawList->AddCircleFilled(Point, 4.0f, IM_COL32(255, 220, 0, 255));
				}
			}
		}

		ImGui::Begin("Physics Debug");

		RECT rc; GetClientRect(hWnd, &rc);
		ImGui::Text("aspect %.4f  (화면 x 범위 = +-%.4f)",
			(float)(rc.right - rc.left) / (rc.bottom - rc.top),
			(float)(rc.right - rc.left) / (rc.bottom - rc.top));

		ImGui::Checkbox("Pause", &bPausePhysics);
		ImGui::SameLine();
		if (ImGui::Button("Step"))
		{
			// 다음 프레임 물리 구간에서 소비된다
			bStepOnce = true;
		}
		ImGui::SameLine();
		ImGui::Checkbox("Draw Contacts", &bDrawContacts);
		ImGui::SameLine();
		ImGui::Checkbox("Draw Colliders", &bDrawColliders);
		ImGui::SliderFloat("Normal Length", &NormalLength, 10.0f, 120.0f);
		ImGui::Checkbox("Warm Starting", &CM.bWarmStarting);

		ImGui::SeparatorText("Solver");
		ImGui::SliderInt("Velocity Iter", &CM.velocityIterations, 1, 20);
		ImGui::SliderInt("Position Iter", &CM.positionIterations, 1, 20);
		ImGui::SliderFloat("Baumgarte", &CM.baumgarte, 0.05f, 1.0f);
		ImGui::SliderFloat("Slop", &CM.slop, 0.0f, 0.02f, "%.4f");
		ImGui::SliderFloat("Rolling", &CM.rollingResistance, 0.0f, 0.02f, "%.4f");

		// slop 근처에서 평평하면 수렴한 것
		ImGui::Text("max penetration %.5f  (slop %.5f)", CM.maxPenetration, CM.slop);
		{
			static float PenHistory[240] = {};
			static int PenIndex = 0;
			PenHistory[PenIndex] = CM.maxPenetration;
			PenIndex = (PenIndex + 1) % 240;
			ImGui::PlotLines("##pen", PenHistory, 240, PenIndex, nullptr, 0.0f, 0.02f, ImVec2(0, 60));
		}

		ImGui::SeparatorText("Sleep");
		ImGui::Checkbox("Sleep Enabled", &CM.bSleepEnabled);
		{
			int SleepingCount = 0;
			int DynamicCount = 0;
			for (ACollider* c : CM.colliders)
			{
				if (c->GetMass() <= 0.0f) continue;
				DynamicCount++;
				if (c->IsSleeping()) SleepingCount++;
			}
			ImGui::SameLine();
			ImGui::Text("%d / %d", SleepingCount, DynamicCount);
		}
		ImGui::SliderFloat("Linear Tol", &CM.linearSleepTolerance, 0.0f, 0.1f, "%.4f");
		ImGui::SliderFloat("Angular Tol", &CM.angularSleepTolerance, 0.0f, 0.3f, "%.4f");
		ImGui::SliderFloat("Time To Sleep", &CM.timeToSleep, 0.05f, 2.0f);

		// 안 잠들 때 누가 붙잡고 있는지. 임계값을 못 넘는 물체 하나가 무리 전체를 깨워둔다
		for (ACollider* c : CM.colliders)
		{
			if (c->GetMass() <= 0.0f) continue;

			float speed = c->GetVelocity().Length();

			ImGui::Text("ID %2d %s  v=%.4f %s  w=%+.4f %s  t=%.2f",
				c->GetID(), c->IsSleeping() ? "zzz" : "   ",
				speed, speed < CM.linearSleepTolerance ? "ok" : "  ",
				c->GetAngularVelocity(), fabsf(c->GetAngularVelocity()) < CM.angularSleepTolerance ? "ok" : "  ",
				c->GetSleepTimer());
		}

		ImGui::SeparatorText("Contacts");
		ImGui::Text("count: %d", (int)CM.debugContacts.size());
		for (int i = 0; i < (int)CM.debugContacts.size(); i++)
		{
			const CollisionInfo& Contact = CM.debugContacts[i];

			for (int k = 0; k < Contact.pointCount; k++)
			{
				const ContactPoint& Point = Contact.points[k];
				ImGui::Text("[%2d.%d] id=%08X  n=(%+.2f, %+.2f)  pen=%.4f  Pn=%.3f  Pt=%+.3f",
					i, k, Point.id, Contact.normal.x, Contact.normal.y,
					Point.penetration, Point.normalImpulse, Point.tangentImpulse);
			}
		}

		ImGui::SeparatorText("Bodies");

		// slip = 접촉점에서 실제로 미끄러지는 속도. 0이면 구르는 중이라 마찰이 할 일이 없다
		for (ACollider* c : CM.colliders)
		{
			if (c->GetMass() <= 0.0f) continue;                    // 정적 제외
			ACircle* circle = dynamic_cast<ACircle*>(c);
			if (!circle) continue;                                 // 원만

			ImGui::Text("ID %2d  v=(%+.3f, %+.3f)  w=%+8.3f  slip=%+.5f",
				c->GetID(), c->GetVelocity().x, c->GetVelocity().y,
				c->GetAngularVelocity(),
				c->GetVelocity().x + c->GetAngularVelocity() * circle->GetRadius());
		}

		// 전체 운동에너지. 잦아들면 수렴, 평평하면 진동, 오르면 발산이다.
		{
			float energy = 0.0f;
			for (ACollider* c : CM.colliders)
			{
				if (c->GetMass() <= 0.0f) continue;

				energy += 0.5f * c->GetMass() * c->GetVelocity().LengthSquared()
					+ 0.5f * c->GetInertia() * c->GetAngularVelocity() * c->GetAngularVelocity();
			}

			static float EnergyHistory[240] = {};
			static int EnergyIndex = 0;
			EnergyHistory[EnergyIndex] = energy;
			EnergyIndex = (EnergyIndex + 1) % 240;

			ImGui::Text("kinetic energy %.5f", energy);
			ImGui::PlotLines("##energy", EnergyHistory, 240, EnergyIndex, nullptr, 0.0f, FLT_MAX, ImVec2(0, 60));
		}

		ImGui::End();

		ImGui::Begin("Screen Info");
		ImGui::Text("Mouse Coord : %d %d", MouseX, MouseY);
		ImGui::Text("Mouse Loc : {%f, %f, %f}", WorldMouseXY.x, WorldMouseXY.y, WorldMouseXY.z);
		ImGui::Text("PressedColliderID %s", s.c_str());
		ImGui::Text("ID %d", PressedCollider ? PressedCollider->GetID() : -1);
		//FVector TipLoc = gameManager.GetSlingShot()->GetBackBand()->TipLocation;
		//ImGui::Text("TipLoc : (%f %f %f)", TipLoc.x, TipLoc.y, TipLoc.z);
		ImGui::Text("GameState : %d", static_cast<int>(gameManager.GetGameState()));
		ImGui::Text("Bird %d, Pig %d", gameManager.GetBirdCount(), gameManager.GetPigCount());
		ImGui::SetNextItemWidth(100);
		ImGui::SetNextItemWidth(100);
		ImGui::End();

		ImGui::Begin("Castle Editor");
		ImGui::InputFloat("CastleWidth", &BlockWidth);
		ImGui::InputFloat("CastleHeight", &BlockHeight);
		if (ImGui::Button("Rotate", ImVec2(100, 20)))
		{
			std::swap(BlockWidth, BlockHeight);
		}
		if (ImGui::Button("Spawn Box", ImVec2(100, 20)))
		{
			ABlock* Block = SpawnColider<ABlock>({ 0, 0, 0 }, EPrimitive::Rectangle, true, { BlockWidth, BlockHeight, 0 }, 70);
			Block->bEditing = true;
		}
		ImGui::InputFloat("PigWidth", &PigWidth);
		ImGui::InputFloat("PigHeight", &PigHeight);
		if (ImGui::Button("Spawn Pig", ImVec2(100, 20)))
		{
			APig* Pig = SpawnColider<APig>({ 0, 0, 0 }, EPrimitive::Circle, true, { PigWidth, PigHeight, 0 }, 30);
			Pig->bEditing = true;
		}
		if (ImGui::Button("Clear Map", ImVec2(100, 20)))
		{
			LoadManager.ClearMap();
		}
		int BirdCount = 3;
		ImGui::InputInt("Bird Count on This Level", &BirdCount);
		if (ImGui::Button("Save Map", ImVec2(100, 20)))
		{
			LoadManager.SaveMap(BirdCount);
		}
		if (ImGui::Button("Delete Select Object", ImVec2(100, 20)))
		{
			PressedCollider->Destroy();
		}
		if (ImGui::Button("Restart", ImVec2(100, 20)))
		{
			gameManager.Restart();
		}

		ImGui::SetNextItemWidth(200);
		ImGui::SetNextItemWidth(300);
		ImGui::End();

		ImGui::Render();										// 그리기 명령 준비	
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	// 그리기 명령 실행

		// 프레임 교체
		renderer.SwapBuffer();
		gameManager.CheckGameState();

		do	// 프레임 대기
		{
			Sleep(0);
			QueryPerformanceCounter(&endTime);

			// 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환)
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
		} while (elapsedTime < targetFrameTime);

		// 크게 튄 프레임을 한 번에 갚으려다 더 밀리는 악순환을 막는다.
		// 대신 물리가 잠깐 느려진다.
		elapsedTime = min(elapsedTime, maxAccumulated * 1000.0);
	}

	//ImGui 리소스 해제
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// 렌더러 리소스 해제
	renderer.ReleaseVertexBuffers();
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();

	// 배경 비트맵 해제
	if (InGameBackgroundBitmap)
	{
		InGameBackgroundBitmap->Release();
		InGameBackgroundBitmap = nullptr;
	}

	// 사운드 매니저 해제
	SM.Shutdown();

	return 0;
}