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

// ScreenToWorld의 역변환. 물리 값은 월드 좌표인데 ImGui는 화면 좌표로 그리므로 필요하다.
// 크기를 GetClientRect가 아니라 ImGui의 DisplaySize에서 가져오는 게 중요하다.
// DPI 배율이 걸린 화면에서는 둘이 다르고(예: 150%면 1.5배), 물리 픽셀을 넘기면
// 그린 게 그만큼 밀린다. 종횡비는 배율과 무관하니 aspect는 그대로 쓸 수 있다.
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

	LARGE_INTEGER frequency;	// tick/sec
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0;

	bool bIsExit = false;
	bool bPressed = false;
	int MouseX = 0, MouseY = 0;
	FVector WorldMouseXY;
	ACollider* PressedCollider = nullptr;

	float BlockWidth = 0.4;
	float BlockHeight = 0.05;
	float PigWidth = 0.15, PigHeight = 0.15;
	bool bEditorMode = false;

	// 물리 디버그
	bool bPausePhysics = false;		// 켜면 물리가 멈춘다 (렌더와 UI는 계속 돈다)
	bool bStepOnce = false;			// Step 버튼이 눌린 프레임에만 한 번 진행
	bool bDrawContacts = true;		// 접촉점/법선 그리기
	bool bDrawColliders = true;		// 사각형 콜라이더의 OBB 외곽선 그리기
	float NormalLength = 40.0f;		// 법선 표시 길이 (픽셀)

	// 매니저 초기화
	GameManager& gameManager = GameManager::GetInstance();
	gameManager.Initialize();

	UIManager& uiManager = UIManager::GetInstance();
	uiManager.Initialize(renderer, windowWidth, windowHeight);

	UObjectManager& ObjectManager = UObjectManager::GetInstance();
	CollisionManager& CM = CollisionManager::GetInstance();
	LoadManager& LoadManager = LoadManager::Get();

	SoundManager& SM = SoundManager::GetInstance();
	if (!SM.Initialize())
	{
		return 0;
	}

	// 루프 진입 전 필요한 리소스 생성
	SM.LoadSound("bgm_main", L"Assets/bgm_main.wav");
	SM.LoadSound("sfx_bird", L"Assets/sfx_bird.wav");

	SM.PlayBGM("bgm_main", true, 0.5f);

	// 인게임 배경화면 로드
	ID2D1Bitmap* InGameBackgroundBitmap = renderer.LoadBitmapFromFile(L"Assets/img/ingamebackground.jpg");

	/*bool bResult = LoadManager.LoadMap(0);
	if (!bResult)
	{
		gameManager.SpawnBirdAndSlingShot();
	}*/

	// 화면 경계 벽 (Clear Map 해도 ClearMap()이 다시 만든다)
	gameManager.SpawnWalls();

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

		gameManager.CheckGameState();

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
							//원 모양 객체를 클릭 중
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
							//직사각형 모양의 객체를 클릭 중
							PressedCollider = Collider;
							Collider->Clicked();
							bFound = true;
							break;
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

		// 일시정지 중에는 Step을 누른 프레임에만 한 번 진행한다.
		// 물리를 멈춰도 아래 렌더와 ImGui는 계속 도니까, 터지는 순간의 접촉을
		// 화면에 띄워놓고 들여다볼 수 있다.
		bool bAdvancePhysics = !bPausePhysics || bStepOnce;
		bStepOnce = false;

		uiManager.Update(elapsedTime * 0.001);


		for (ACollider* Collider : CM.colliders)
		{
			Collider->Move(elapsedTime);
		}

		if (bAdvancePhysics)
		{
			for (ACollider* Collider : CM.colliders)
			{
				Collider->Move(elapsedTime);
			}

			// 충돌 검사
			uiManager.GetCollisionInfos(CM.CheckCollisionAll());
		}

		//

		//매 프레임 UObject에 Tick 호출
		for (int i = ObjectManager.AllObjects.size() - 1; i >= 0; --i)
		{
			ObjectManager.AllObjects[i]->Tick(elapsedTime * 0.001);
		}

		// 렌더 준비
		renderer.Prepare();

		// 배경화면 그리기 (모든 게임 객체 뒤에 먼저 렌더링)
		if (InGameBackgroundBitmap)
		{
			renderer.DrawBitmap(InGameBackgroundBitmap, 0.0f, 0.0f, (float)windowWidth, (float)windowHeight);
		}

		renderer.PrepareShader();

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
		uiManager.Render(4);

		// ImGui
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 사각형 콜라이더의 OBB 외곽선.
		// 이제 물리도 이 OBB를 쓴다. 외곽선과 스프라이트가 어긋나면 물리도 같이 어긋난다.
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

				for (int i = 0; i < 4; i++)
				{
					DrawList->AddLine(WorldToScreen(Box.vertex[i]),
						WorldToScreen(Box.vertex[(i + 1) % 4]),
						IM_COL32(80, 200, 255, 255), 2.0f);
				}

				// 꼭짓점 0이 어디인지 표시. 블록을 돌리면 이 점도 같이 돌아야 한다.
				DrawList->AddCircleFilled(WorldToScreen(Box.vertex[0]), 4.0f, IM_COL32(80, 200, 255, 255));
			}
		}

		// 접촉점과 법선 그리기.
		// 배경 draw list라서 게임 화면 위, ImGui 창 아래에 그려진다.
		if (bDrawContacts)
		{
			ImDrawList* DrawList = ImGui::GetBackgroundDrawList();

			for (const CollisionInfo& Contact : CM.debugContacts)
			{
				// 법선은 접촉점들이 공유하고, 점만 여러 개일 수 있다
				for (int i = 0; i < Contact.pointCount; i++)
				{
					ImVec2 Point = WorldToScreen(Contact.points[i].position);

					// 법선은 방향이라 위치와 달리 y를 뒤집기만 하면 된다.
					// (화면 y는 아래로, 월드 y는 위로 증가)
					ImVec2 Tip = ImVec2(Point.x + Contact.normal.x * NormalLength,
						Point.y - Contact.normal.y * NormalLength);

					// 법선은 B -> A 방향. 즉 A를 밀어내는 쪽을 가리켜야 한다.
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

		// 물리 디버깅 창
		for (ACollider* c : CollisionManager::GetInstance().colliders)
		{
			if (c->GetMass() <= 0.0f) continue;                    // 정적 제외
			ACircle* circle = dynamic_cast<ACircle*>(c);
			if (!circle) continue;                                 // 원만

			float slip = c->GetVelocity().x + c->GetAngularVelocity() * circle->GetRadius();
			ImGui::Text("ID %2d  v=(%+.3f, %+.3f)  w=%+8.3f  slip=%+.5f",
				c->GetID(), c->GetVelocity().x, c->GetVelocity().y,
				c->GetAngularVelocity(), slip);

			static float hist[240] = {};
			static int idx = 0;
			hist[idx] = slip;
			idx = (idx + 1) % 240;
			ImGui::PlotLines("slip", hist, 240, idx, nullptr, -3.0f, 3.0f, ImVec2(0, 80));

			float energy = 0.0f;
			for (ACollider* c : CollisionManager::GetInstance().colliders)
				if (c->GetMass() > 0.0f)
					energy += 0.5f * c->GetMass() * c->GetVelocity().LengthSquared()
					+ 0.5f * c->GetInertia() * c->GetAngularVelocity() * c->GetAngularVelocity();
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
		if (ImGui::Checkbox("EditorMode", &bEditorMode))
		{
			CollisionManager::GetInstance().SetAllCollisionFriction(1.f, 1.f);
		}
		else
		{
			CollisionManager::GetInstance().SetAllCollisionFriction(0.3f, 0.5f);
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

		do	// 프레임 대기
		{
			Sleep(0);
			QueryPerformanceCounter(&endTime);

			// 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환)
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
		} while (elapsedTime < targetFrameTime);
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
