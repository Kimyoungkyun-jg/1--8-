#pragma comment(lib, "user32")				

#include <windows.h>					
#include <d3d11.h>						
#include <d3dcompiler.h>
#include <vector>

#include "ImGui/imgui.h"				
#include "ImGui/imgui_internal.h"		
#include "ImGui/imgui_impl_dx11.h"		
#include "ImGui/imgui_impl_win32.h"		

#include <vector>

using namespace std;

// FVertexSimple, triangle_vertices, cube_vertices, sphere_vertices
#include "Sphere.h"	
#include "Vector.h"
#include "Renderer.h"
#include "UObject.h"
#include "Global.h"


//모든 매니저 헤더파일
#include "TotalManager.h"
#include "UIManager.h"
#include "CollisionManager.h"

UIManager* uiManager = nullptr; //전역으로 사용하는 매니저


bool bUseGravity = true;
int UObject::TotalUObject = 0;

//class UPrimitiveManager
//{
//public:
//	// 반드시 UBall이 아닌 UPrimitive로 선언하여야 하며 바꾸면 안됩니다.
//	UObject** AllObjects;
//	int Capacity;
//
//	UPrimitiveManager()
//	{
//		AllObjects = new UObject * [10];
//		Capacity = 10;
//	}
//
//	~UPrimitiveManager()
//	{
//		int count = UBall::TotalNumBalls;
//
//		for (int i = 0; i < count; i++)
//		{
//			delete AllObjects[i];
//		}
//
//		delete[] AllObjects;
//	}
//
//	// 임의의 위치, 속도, 크기를 가진 공 생성
//	UBall* CreateRandomBall(ID3D11Buffer* vertexBuffer, UINT numVertices)
//	{
//		// new 연산자를 사용해 공의 인스턴스(Instance)를 즉시 생성합니다.
//		UBall* ball = new UBall();
//
//		// 크기
//		float minRadius = 0.05f;
//		float maxRadius = 0.10f;
//		ball->Radius = (static_cast<float>(rand()) / RAND_MAX) * (maxRadius - minRadius) + minRadius;
//
//		// 질량
//		float PI = acos(-1.0f);
//		ball->Mass = 4.0f / 3.0f * PI * (ball->Radius * ball->Radius * ball->Radius);
//
//		// 위치
//		float locWidth = 0.5f;
//		float locHeight = 0.5f;
//		float locX = (static_cast<float>(rand()) / RAND_MAX) * locWidth - locWidth / 2;
//		float locY = (static_cast<float>(rand()) / RAND_MAX) * locHeight - locHeight / 2;
//		ball->Location = FVector(locX, locY);
//
//		// 속력
//		float minSpeed = 1.0f;
//		float maxSpeed = 5.0f;
//		float speed = (static_cast<float>(rand()) / RAND_MAX) * (maxSpeed - minSpeed) + minSpeed;
//
//		// 방향
//		float radian = (static_cast<float>(rand()) / RAND_MAX) * 2 * PI;
//		FVector direction = FVector(cos(radian), sin(radian));
//
//		// 속도
//		ball->Velocity = direction * speed;
//
//		// 인자로 받은 렌더링 데이터를 멤버 변수에 저장합니다.
//		ball->VertexBuffer = vertexBuffer;
//		ball->NumVertices = numVertices;
//
//		return ball;
//	}
//
//	void RemoveBall(int removeIndex)
//	{
//		int lastIndex = UBall::TotalNumBalls - 1;
//
//		// delete 연산자를 사용해 공의 인스턴스를 즉시 소멸시킵니다.
//		delete AllObjects[removeIndex];
//
//		// 마지막 요소를 삭제된 자리에 덮어쓰기
//		AllObjects[removeIndex] = AllObjects[lastIndex];
//		AllObjects[lastIndex] = nullptr;
//	}
//
//	void ResizeBallList(int newCount, ID3D11Buffer* vertexBuffer, UINT numVertices)
//	{
//		if (newCount < UBall::TotalNumBalls)
//		{
//			int removeCount = UBall::TotalNumBalls - newCount;
//			for (int i = 0; i < removeCount; i++)
//			{
//				// 관리되고 있는 전체 공들 중 반드시 임의의(Random) 공이 소멸해야 합니다.
//				int removeIndex = rand() % UBall::TotalNumBalls;
//
//				RemoveBall(removeIndex);
//			}
//		}
//		else if (newCount > UBall::TotalNumBalls)
//		{
//			Reserve(newCount);
//
//			// 새로 생성
//			for (int i = UBall::TotalNumBalls; i < newCount; i++)
//			{
//				AllObjects[i] = CreateRandomBall(vertexBuffer, numVertices);
//			}
//		}
//	}
//
//	void Reserve(int capacity)
//	{
//		if (Capacity >= capacity)
//		{
//			return;
//		}
//
//		int newCapacity = (Capacity > 0) ? Capacity : 10;
//		while (newCapacity < capacity)
//		{
//			newCapacity *= 2;
//		}
//
//		UPrimitive** AllObjects = new UPrimitive * [newCapacity];
//
//		for (int i = 0; i < UBall::TotalNumBalls; i++)
//		{
//			AllObjects[i] = AllObjects[i];
//		}
//
//		delete[] AllObjects;
//
//		AllObjects = AllObjects;
//		Capacity = newCapacity;
//	}
//};

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
	uiManager = new UIManager();
	uiManager->Initialize(renderer.SwapChain);

	// 정점 개수 계산
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	// 버텍스 버퍼(Vertex Buffer)는 1개만 생성하세요.
	ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	// ImGui 초기화
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	std::vector<UObject> AllObjects;

	const int targetFPS = 144;
	const double targetFrameTime = 1000.0 / targetFPS;	// 한 프레임의 목표 시간 (밀리초 단위)

	LARGE_INTEGER frequency;	// tick/sec
	QueryPerformanceFrequency(&frequency);

	LARGE_INTEGER startTime, endTime;
	double elapsedTime = 0.0;

	int ballCount = 0;

	bool bIsExit = false;
	bool bLeftPressed = false;
	bool bRightPressed = false;



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


		renderer.Prepare();
		renderer.PrepareShader();

		// 여기에 Colider만 이동


		// 충돌 검사

		// 그리기
		for (int i = 0; i < UObject::TotalUObject; i++)
		{
			if (AllObjects.size() < 1) break; //allobject 암것도 없으면 안그림
			

			if (AActor* Actor = dynamic_cast<AActor*>(&AllObjects[i]))
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


		uiManager->Render(4); //UI그리기
		uiManager->Update(elapsedTime * 0.001);


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
	renderer.ReleaseVertexBuffer(vertexBufferCube);
	renderer.ReleaseVertexBuffer(vertexBufferSphere);
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();

	return 0;
}


