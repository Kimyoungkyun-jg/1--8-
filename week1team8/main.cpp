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

// FVertexSimple, triangle_vertices, cube_vertices, sphere_vertices
#include "Vector.h"
#include "Renderer.h"
#include "UObject.h"
#include "Global.h"
#include "TemplateLibrary.h"


//모든 매니저 헤더파일
#include "TotalManager.h"
#include "UIManager.h"
#include "CollisionManager.h"
#include "ObjectManager.h"


bool bUseGravity = true;

float clamp(float val, float minVal, float maxVal)
{
	return fmin(maxVal, fmax(minVal, val));
}

FVector ScreenToWorld(HWND hwnd, int MouseX, int MouseY)
{
	RECT rec;
	GetClientRect(hwnd, &rec);

	float w = rec.right - rec.left;
	float c = rec.top - rec.bottom;

	FVector res;
	res.x = max(-1, min(2 * MouseX / w - 1, 1));
	res.y = max(-1, min(2 * MouseY / c + 1, 1));

	return res;
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

	HWND hWnd = CreateWindowExW(0, WindowClass, Title,
		WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 2560, 1440, nullptr, nullptr, hInstance, nullptr);

	// 렌더러 초기화
	URenderer renderer;
	renderer.Create(hWnd);
	renderer.CreateShader();
	renderer.CreateConstantBuffer();

	//UIManager초기화
	UIManager& uiManager = UIManager::Get();
	uiManager.Initialize(renderer.SwapChain);


	// 버텍스 버퍼(Vertex Buffer)는 1개만 생성하세요.
	renderer.CreateVertexBufferInfos();

	// ImGui 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

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

	UObjectManager &ObjectManager = UObjectManager::Get();
	CollisionManager& CM = CollisionManager::GetInstance();


	//테스트용
	/*AObstacle* block = SpawnColider<AObstacle>(FVector(0, -10, 0), EPrimitive::Rectangle, true, { 0.5,0.5,0.5 });
	block->SetVelocity(0);

	ACollider* NewBall = SpawnColider<ACollider>(FVector(0, 0, 0), EPrimitive::Circle, true, { 0.1, 0.1, 1 });*/

	// Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	ASlingShot *SlingShot = SpawnColider<ASlingShot>({ -0.5, -1.0, 0 }, EPrimitive::Rectangle, false, { 0.05, 0.3, 1 });
	ABird * Bird = SpawnColider<ABird>({ -0.5, -0.5, 0 }, EPrimitive::Circle, false, { 0.05, 0.05, 0.1 });

	SlingShot->EquippedBird = Bird;
	SlingShot->ShotPoint = Bird->GetLocation();
	//AActor* ShotPoint = SpawnActor<AActor>(SlingShot->ShotPoint, EPrimitive::Circle);

	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);

		//입력 처리
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
				MouseX = GET_X_LPARAM(msg.lParam);
				MouseY = GET_Y_LPARAM(msg.lParam);
				WorldMouseXY = ScreenToWorld(hWnd, MouseX, MouseY);
				SetCapture(msg.hwnd);

				bool bFound = false;
				for (ACollider* Collider : CM.colliders)
				{
					FVector ColLoc = Collider->GetLocation();
					EPrimitive Primitive = Collider->GetPrimitive();
					if (Primitive == EPrimitive::Circle)
					{
						float dist = (ColLoc - WorldMouseXY).Length();
						if (dist <= Collider->GetScale().x)
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
						float half = Collider->GetScale().x / 2.f;
						if (WorldMouseXY.x >= ColLoc.x - half
							&& WorldMouseXY.x <= ColLoc.x + half
							&& WorldMouseXY.y >= ColLoc.y - half
							&& WorldMouseXY.y <= ColLoc.y + half
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
				MouseX = GET_X_LPARAM(msg.lParam);
				MouseY = GET_Y_LPARAM(msg.lParam);
				WorldMouseXY = ScreenToWorld(hWnd, MouseX, MouseY);

				if (PressedCollider)
				{
					if (PressedCollider->GetColliderId() == EColliderId::BIRD)
					{
						SlingShot->Released(WorldMouseXY);
					}
					else PressedCollider->Released(WorldMouseXY);
				}

				bPressed = false;
				ReleaseCapture();
			}
			else if (msg.message == WM_MOUSEMOVE)
			{
				if (bPressed)
				{
					MouseX = GET_X_LPARAM(msg.lParam);
					MouseY = GET_Y_LPARAM(msg.lParam);
					WorldMouseXY = ScreenToWorld(hWnd, MouseX, MouseY);
					if (PressedCollider)
					{
						PressedCollider->Pressed(WorldMouseXY);
					}
				}
			}
		}

		renderer.Prepare();
		renderer.PrepareShader();

		for (ACollider *Collider : CM.colliders)
		{
			Collider->Move(elapsedTime);
		}

		// 충돌 검사
		CollisionManager& ColManager = CollisionManager::GetInstance();
		ColManager.CheckCollisionAll();

		

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
			case EColliderId::SLINGSHOT:
				s = "SLINGSHOT";
				break;
			case EColliderId::NONE:
				s = "NONE";
				break;
			default:
				s = "NONE";
				break;
			}
		}
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");
		ImGui::SetNextItemWidth(100);
		ImGui::SetNextItemWidth(100);
		ImGui::End();

		ImGui::Begin("Screen Info");
		ImGui::Text("Mouse Coord : %d %d", MouseX, MouseY);
		ImGui::Text("Mouse Loc : {%f, %f, %f}", WorldMouseXY.x, WorldMouseXY.y, WorldMouseXY.z);
		ImGui::Text("PressedColliderID %s", s.c_str());
		ImGui::Text("ID %d", PressedCollider ? PressedCollider->GetID() : -1);
		ImGui::End();


		uiManager.Render(4); //UI그리기
		uiManager.Update(elapsedTime * 0.001);


		ImGui::Render();										// 그리기 명령 준비	
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	// 그리기 명령 실행

		renderer.SwapBuffer();

		do	// 프레임 대기
		{
			Sleep(0);
			QueryPerformanceCounter(&endTime);

			// 한 프레임이 소요된 시간 계산 (밀리초 단위로 변환)
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
		} while (elapsedTime < targetFrameTime);
	}

	ImGui_ImplDX11_Shutdown();	//ImGui 리소스 해제
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// 렌더러 리소스 해제
	renderer.ReleaseVertexBuffers();
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();

	return 0;
}


