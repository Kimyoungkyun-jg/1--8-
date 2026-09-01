#pragma comment(lib, "user32")				

#include <windows.h>					
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
		CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024, nullptr, nullptr, hInstance, nullptr);

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

	int ballCount = 1;

	bool bIsExit = false;
	bool bLeftPressed = false;
	bool bRightPressed = false;

	UObjectManager &ObjectManager = UObjectManager::Get();


	//테스트용
	AObstacle* block = SpawnColider<AObstacle>(FVector(0, -10, 0), EPrimitive::Rectangle, { 0.5,0.5,0.5 });
	block->SetVelocity(0);

	ACollider* NewBall = SpawnColider<ACollider>(FVector(0, 10, 0), EPrimitive::Circle, { 0.1, 0.1, 1 });

	// Main Loop (Quit Message가 들어오기 전까지 아래 Loop를 무한히 실행하게 됨)
	while (bIsExit == false)
	{
		QueryPerformanceCounter(&startTime);

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
			else if (msg.message == WM_KEYDOWN)
			{
				if (msg.wParam == VK_LEFT)
				{
					bLeftPressed = true;
				}
				else if (msg.wParam == VK_RIGHT)
				{
					bRightPressed = true;
				}
			}
			else if (msg.message == WM_KEYUP)
			{
				if (msg.wParam == VK_LEFT)
				{
					bLeftPressed = false;
				}
				else if (msg.wParam == VK_RIGHT)
				{
					bRightPressed = false;
				}
			}
		}

		//볼 생성




		renderer.Prepare();
		renderer.PrepareShader();

		//TODO: ColiderManager의 Colider모음으로 순회
		for ( int i = 0; i < ObjectManager.AllObjects.size ( ); i++ )
		{
			if (ACollider* Colider = dynamic_cast< ACollider* > (ObjectManager.AllObjects[i]))
			{
				Colider->Move(elapsedTime, true);
			}
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


		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");
		ImGui::Checkbox("Gravity", &bUseGravity);
		ImGui::SetNextItemWidth(100);
		ImGui::SetNextItemWidth(100);
		if (ImGui::InputInt("Number of Balls", &ballCount, 1))
		{
			if (ballCount < 0)
			{
				ballCount = 0;
			}

			//manager.ResizeBallList(ballCount, vertexBufferSphere, numVerticesSphere);
		}

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


