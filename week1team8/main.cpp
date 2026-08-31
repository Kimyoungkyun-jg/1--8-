#pragma comment(lib, "user32")				

#include <windows.h>					
#include <d3d11.h>						
#include <d3dcompiler.h>			

#include "ImGui/imgui.h"				
#include "ImGui/imgui_internal.h"		
#include "ImGui/imgui_impl_dx11.h"		
#include "ImGui/imgui_impl_win32.h"		

// FVertexSimple, triangle_vertices, cube_vertices, sphere_vertices
#include "Sphere.h"	
#include "Vector.h"
#include "Renderer.h"

// 화면 경계 (NDC)
const float leftBorder = -1.0f;
const float rightBorder = 1.0f;
const float topBorder = 1.0f;
const float bottomBorder = -1.0f;

// 중력
const FVector G(0.0f, -9.8f, 0.0f);
bool bUseGravity = true;

// 클래스 이름 외에는 자유롭게 수정하세요.
class UPrimitive
{
public:
	// 렌더링
	ID3D11Buffer* VertexBuffer;
	UINT NumVertices;

	virtual ~UPrimitive() {}
	virtual void Move(float t) = 0;							// t 시간동안 이동
	virtual void Draw(URenderer& renderer) = 0;				// 화면에 그리기
	virtual bool CheckCollision(UPrimitive* other) = 0;		// 충돌 감지
	virtual void ResolveCollision(UPrimitive* other) = 0;	// 충돌 해결 (속도 변화, 겹침 해결)
};

// 클래스 이름과, 아래 다섯개의 변수 이름은 변경하지 않습니다.
class UBall : public UPrimitive
{
public:
	FVector Location;			// 위치
	FVector Velocity;			// 속도
	float Radius;				// 반지름
	float Mass;					// 질량
	static int TotalNumBalls;	// 공 개수 (생성자, 파괴자 관리)

	UBall()
	{
		TotalNumBalls++;
	}

	~UBall() override
	{
		TotalNumBalls--;
	}

	void Move(float t) override
	{
		float deltaTime = t / 1000.0f;

		// 속도 변화
		if (bUseGravity)
		{
			Velocity += G * deltaTime;
		}

		// 위치 변화
		Location += Velocity * deltaTime;

		// 공-벽 충돌 감지 및 해결
		if (Location.x < leftBorder + Radius)
		{
			Velocity.x *= -1.0f;
			Location.x = leftBorder + Radius;
		}
		if (Location.x > rightBorder - Radius)
		{
			Velocity.x *= -1.0f;
			Location.x = rightBorder - Radius;
		}
		if (Location.y < bottomBorder + Radius)
		{
			Velocity.y *= -1.0f;
			Location.y = bottomBorder + Radius;
		}
		if (Location.y > topBorder - Radius)
		{
			Velocity.y *= -1.0f;
			Location.y = topBorder - Radius;
		}
	}

	void Draw(URenderer& renderer) override
	{
		// 위치와 크기를 상수 버퍼에 업데이트
		renderer.UpdateConstant(Location, FVector(Radius, Radius, 1.0f));
		renderer.RenderPrimitive(VertexBuffer, NumVertices);
	}

	bool CheckCollision(UPrimitive* other) override
	{
		UBall* otherBall = dynamic_cast<UBall*>(other);
		if (!otherBall)
		{
			return false;
		}

		FVector diff = Location - otherBall->Location;
		float radiusSum = (Radius + otherBall->Radius);

		// 두 공 사이의 거리가 반지름의 합보다 작으면 충돌
		return diff.LengthSquared() < radiusSum * radiusSum;
	}

	void ResolveCollision(UPrimitive* other) override
	{
		UBall* otherBall = dynamic_cast<UBall*>(other);
		if (!otherBall)
		{
			return;
		}

		FVector diff = Location - otherBall->Location;

		// 충돌 법선 벡터
		FVector normal = diff;
		normal.Normalize();

		// 상대 속도
		FVector v_rel = Velocity - otherBall->Velocity;

		// 상대 속도와 법선이 이루는 각도
		float v_rel_n = v_rel.DotProduct(normal);

		// 멀어지고 있다면 충돌 처리를 하지 않음
		if (v_rel_n >= 0)
		{
			return;
		}

		// 반발 계수
		float e = 1.0f;

		// 충격량
		float j = -(1.0f + e) * v_rel_n / (1.0f / Mass + 1.0f / otherBall->Mass);

		// 속도 변화
		Velocity += normal * (j / Mass);
		otherBall->Velocity -= normal * (j / otherBall->Mass);

		// 겹침 해결
		float penetration = (Radius + otherBall->Radius) - diff.Length();

		if (penetration < 0.01f)
		{
			return;
		}

		FVector correction = normal * (penetration / (1.0f / Mass + 1.0f / otherBall->Mass));
		Location += correction * (1.0f / Mass);
		otherBall->Location -= correction * (1.0f / otherBall->Mass);
	}
};

int UBall::TotalNumBalls = 0;

class UPrimitiveManager
{
public:
	// 반드시 UBall이 아닌 UPrimitive로 선언하여야 하며 바꾸면 안됩니다.
	UPrimitive** PrimitiveList;
	int Capacity;

	UPrimitiveManager()
	{
		PrimitiveList = new UPrimitive * [10];
		Capacity = 10;
	}

	~UPrimitiveManager()
	{
		int count = UBall::TotalNumBalls;

		for (int i = 0; i < count; i++)
		{
			delete PrimitiveList[i];
		}

		delete[] PrimitiveList;
	}

	// 임의의 위치, 속도, 크기를 가진 공 생성
	UBall* CreateRandomBall(ID3D11Buffer* vertexBuffer, UINT numVertices)
	{
		// new 연산자를 사용해 공의 인스턴스(Instance)를 즉시 생성합니다.
		UBall* ball = new UBall();

		// 크기
		float minRadius = 0.05f;
		float maxRadius = 0.10f;
		ball->Radius = (static_cast<float>(rand()) / RAND_MAX) * (maxRadius - minRadius) + minRadius;

		// 질량
		float PI = acos(-1.0f);
		ball->Mass = 4.0f / 3.0f * PI * (ball->Radius * ball->Radius * ball->Radius);

		// 위치
		float locWidth = 0.5f;
		float locHeight = 0.5f;
		float locX = (static_cast<float>(rand()) / RAND_MAX) * locWidth - locWidth / 2;
		float locY = (static_cast<float>(rand()) / RAND_MAX) * locHeight - locHeight / 2;
		ball->Location = FVector(locX, locY);

		// 속력
		float minSpeed = 1.0f;
		float maxSpeed = 5.0f;
		float speed = (static_cast<float>(rand()) / RAND_MAX) * (maxSpeed - minSpeed) + minSpeed;

		// 방향
		float radian = (static_cast<float>(rand()) / RAND_MAX) * 2 * PI;
		FVector direction = FVector(cos(radian), sin(radian));

		// 속도
		ball->Velocity = direction * speed;

		// 인자로 받은 렌더링 데이터를 멤버 변수에 저장합니다.
		ball->VertexBuffer = vertexBuffer;
		ball->NumVertices = numVertices;

		return ball;
	}

	void RemoveBall(int removeIndex)
	{
		int lastIndex = UBall::TotalNumBalls - 1;

		// delete 연산자를 사용해 공의 인스턴스를 즉시 소멸시킵니다.
		delete PrimitiveList[removeIndex];

		// 마지막 요소를 삭제된 자리에 덮어쓰기
		PrimitiveList[removeIndex] = PrimitiveList[lastIndex];
		PrimitiveList[lastIndex] = nullptr;
	}

	void ResizeBallList(int newCount, ID3D11Buffer* vertexBuffer, UINT numVertices)
	{
		if (newCount < UBall::TotalNumBalls)
		{
			int removeCount = UBall::TotalNumBalls - newCount;
			for (int i = 0; i < removeCount; i++)
			{
				// 관리되고 있는 전체 공들 중 반드시 임의의(Random) 공이 소멸해야 합니다.
				int removeIndex = rand() % UBall::TotalNumBalls;

				RemoveBall(removeIndex);
			}
		}
		else if (newCount > UBall::TotalNumBalls)
		{
			Reserve(newCount);

			// 새로 생성
			for (int i = UBall::TotalNumBalls; i < newCount; i++)
			{
				PrimitiveList[i] = CreateRandomBall(vertexBuffer, numVertices);
			}
		}
	}

	void Reserve(int capacity)
	{
		if (Capacity >= capacity)
		{
			return;
		}

		int newCapacity = (Capacity > 0) ? Capacity : 10;
		while (newCapacity < capacity)
		{
			newCapacity *= 2;
		}

		UPrimitive** primitiveList = new UPrimitive * [newCapacity];

		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			primitiveList[i] = PrimitiveList[i];
		}

		delete[] PrimitiveList;

		PrimitiveList = primitiveList;
		Capacity = newCapacity;
	}
};

float clamp(float val, float minVal, float maxVal)
{
	return fmin(maxVal, fmax(minVal, val));
}

class UBlock : public UPrimitive
{
public:
	static float Width;
	static float Height;
	static int TotalNumBlocks;

	FVector Location;

	void Move(float t) override {}

	void Draw(URenderer& renderer) override
	{
		renderer.UpdateConstant(Location, FVector(Width, Height, 1.0f));
		renderer.RenderPrimitive(VertexBuffer, NumVertices);
	}

	bool CheckCollision(UPrimitive* other) override
	{
		UBall* ball = dynamic_cast<UBall*>(other);
		if (!ball)
		{
			return false;
		}

		float blockLeft = Location.x - Width / 2;
		float blockRight = Location.x + Width / 2;
		float blockUp = Location.y + Height / 2;
		float blockDown = Location.y - Height / 2;

		float closestX = clamp(ball->Location.x, blockLeft, blockRight);
		float closestY = clamp(ball->Location.y, blockDown, blockUp);

		FVector closest(closestX, closestY);
		FVector diff = closest - ball->Location;

		// 거리가 반지름의 합보다 작으면 충돌
		return diff.LengthSquared() < ball->Radius * ball->Radius;
	}

	void ResolveCollision(UPrimitive* other) override
	{
		UBall* ball = dynamic_cast<UBall*>(other);
		if (!ball)
		{
			return;
		}

		float blockLeft = Location.x - Width / 2;
		float blockRight = Location.x + Width / 2;
		float blockUp = Location.y + Height / 2;
		float blockDown = Location.y - Height / 2;

		float closestX = clamp(ball->Location.x, blockLeft, blockRight);
		float closestY = clamp(ball->Location.y, blockDown, blockUp);

		FVector closest(closestX, closestY);
		FVector diff = closest - ball->Location;

		// 충돌 법선 벡터
		FVector normal = diff;
		normal.Normalize();

		// 상대 속도
		FVector v_rel = ball->Velocity * -1.0f;

		// 상대 속도와 법선이 이루는 각도
		float v_rel_n = v_rel.DotProduct(normal);

		// 멀어지고 있다면 충돌 처리를 하지 않음
		if (v_rel_n >= 0)
		{
			return;
		}

		// 반발 계수
		float e = 1.0f;

		// 충격량
		float j = -(1.0f + e) * v_rel_n;

		// 속도 변화
		ball->Velocity -= normal * j;

		// 겹침 해결
		float penetration = ball->Radius - diff.Length();

		if (penetration < 0.01f)
		{
			return;
		}

		FVector correction = normal * penetration;
		ball->Location -= correction;
	}
};

constexpr int BlockRows = 4;
constexpr int BlockCols = 8;
float UBlock::Height = 0.3f * 2 / BlockRows; // 비율 * 창 높이 / 행
float UBlock::Width = 1.0f * 2 / BlockCols;	// 비율 * 창 너비 / 열
int UBlock::TotalNumBlocks = BlockRows * BlockCols;

class UBlockManager
{
public:
	UPrimitive* BlockList2D[BlockRows][BlockCols];

	~UBlockManager()
	{
		RemoveAllBlocks();
	}

	void CreateBlocks(ID3D11Buffer* vertexBuffer, UINT numVertices)
	{
		for (int r = 0; r < BlockRows; r++)
		{
			for (int c = 0; c < BlockCols; c++)
			{
				UBlock* block = new UBlock;

				block->Location = FVector(
					-1.0f + UBlock::Width / 2 + UBlock::Width * c,
					1.0f - UBlock::Height / 2 - UBlock::Height * r,
					0.0f);

				block->VertexBuffer = vertexBuffer;
				block->NumVertices = numVertices;

				BlockList2D[r][c] = block;
			}
		}
	}

	void RemoveBlock(int row, int col)
	{
		delete BlockList2D[row][col];
		BlockList2D[row][col] = nullptr;
		UBlock::TotalNumBlocks--;
	}

	void RemoveAllBlocks()
	{
		for (int r = 0; r < BlockRows; r++)
		{
			for (int c = 0; c < BlockCols; c++)
			{
				if (BlockList2D[r][c])
				{
					RemoveBlock(r, c);
				}
			}
		}
	}

	void ReFillAllBlocks(ID3D11Buffer* vertexBuffer, UINT numVertices)
	{
		RemoveAllBlocks();
		CreateBlocks(vertexBuffer, numVertices);
		UBlock::TotalNumBlocks = BlockRows * BlockCols;
	}
};

class UPaddle : public UPrimitive
{
public:
	static float Speed;
	static float Width;
	static float Height;

	FVector Location;
	FVector Velocity;

	void Move(float t) override
	{
		float deltaTime = t / 1000.0f;

		Location += Velocity * deltaTime;

		if (Location.x < leftBorder + Width / 2)
		{
			Location.x = leftBorder + Width / 2;
		}
		if (Location.x > rightBorder - Width / 2)
		{
			Location.x = rightBorder - Width / 2;
		}
	}

	void Draw(URenderer& renderer) override
	{
		renderer.UpdateConstant(Location, FVector(Width, Height, 1.0f));
		renderer.RenderPrimitive(VertexBuffer, NumVertices);
	}

	bool CheckCollision(UPrimitive* other) override
	{
		UBall* ball = dynamic_cast<UBall*>(other);
		if (!ball)
		{
			return false;
		}

		float blockLeft = Location.x - Width / 2;
		float blockRight = Location.x + Width / 2;
		float blockUp = Location.y + Height / 2;
		float blockDown = Location.y - Height / 2;

		float closestX = clamp(ball->Location.x, blockLeft, blockRight);
		float closestY = clamp(ball->Location.y, blockDown, blockUp);

		FVector closest(closestX, closestY);
		FVector diff = closest - ball->Location;

		// 거리가 반지름의 합보다 작으면 충돌
		return diff.LengthSquared() < ball->Radius * ball->Radius;
	}

	// 패들에 윗면에 맞은 경우 맞은 위치에 따라 튕기는 방향 조정
	void ResolveCollision(UPrimitive* other) override
	{
		UBall* ball = dynamic_cast<UBall*>(other);
		if (!ball)
		{
			return;
		}

		float overlapX = (Width / 2 + ball->Radius) - fabs(ball->Location.x - Location.x);
		float overlapY = (Height / 2 + ball->Radius) - fabs(ball->Location.y - Location.y);

		float paddleLeft = Location.x - Width / 2;
		float paddleRight = Location.x + Width / 2;
		float paddleUp = Location.y + Height / 2;
		float paddleDown = Location.y - Height / 2;

		if (overlapX > overlapY)
		{
			if (ball->Location.y > Location.y)
			{
				float x = clamp(ball->Location.x, paddleLeft, paddleRight);
				float offset = x - (paddleLeft);
				float degree = offset / Width * 150.0f - 75.0f;
				float PI = acos(-1.0f);
				float radian = degree * PI / 180.0f;
				// -90도가 왼쪽, 0도가 수직, 90도가 오른쪽

				ball->Velocity = FVector(sin(radian), cos(radian), 0.0f) * ball->Velocity.Length();

				// 겹칩 해결
				if (ball->Location.y > Location.y)
				{
					ball->Location.y += overlapY;
				}
				else
				{
					ball->Location.y -= overlapY;
				}
			}
			else
			{
				ball->Velocity.y *= -1;

				// 겹침 해결
				ball->Location.y -= overlapY;
			}
		}
		else
		{
			ball->Velocity.x *= -1;

			// 겹침 해결
			if (ball->Location.x > Location.x)
			{
				ball->Location.x += overlapX;
			}
			else
			{
				ball->Location.x -= overlapX;
			}
		}
	}
};

float UPaddle::Speed = 1.5f;
float UPaddle::Height = 0.07f;
float UPaddle::Width = 0.5f;

UPaddle* CreatePaddle(ID3D11Buffer* vertexBuffer, UINT numVertices)
{
	UPaddle* paddle = new UPaddle;

	paddle->Location = FVector(0.0f, -0.7f, 0.0f);
	paddle->Velocity = FVector(0.0f, 0.0f, 0.0f);

	paddle->VertexBuffer = vertexBuffer;
	paddle->NumVertices = numVertices;

	return paddle;
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

	// 공 배열을 관리하는 클래스
	UPrimitiveManager manager;
	UPrimitive**& ballList = manager.PrimitiveList;

	// 블럭 배열을 관리하는 클래스
	UBlockManager blockManager;
	blockManager.CreateBlocks(vertexBufferCube, numVerticesCube);

	// 패들
	UPaddle* paddle = CreatePaddle(vertexBufferCube, numVerticesCube);

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
	bool bBlockValid = false;
	bool bPaddleValid = false;

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

		paddle->Velocity.x = (bLeftPressed ? -UPaddle::Speed : 0.0f) +
			(bRightPressed ? UPaddle::Speed : 0.0f);

		renderer.Prepare();
		renderer.PrepareShader();

		// 이동
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			ballList[i]->Move(elapsedTime);
		}

		if (bPaddleValid)
		{
			paddle->Move(elapsedTime);
		}

		// 충돌 검사
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			for (int j = i + 1; j < UBall::TotalNumBalls; j++)
			{
				// 두 객체 충돌 발생
				if (ballList[i]->CheckCollision(ballList[j]))
				{
					ballList[j]->ResolveCollision(ballList[i]);
				}
			}

			if (bBlockValid)
			{
				for (int r = 0; r < BlockRows; r++)
				{
					for (int c = 0; c < BlockCols; c++)
					{
						if (blockManager.BlockList2D[r][c] && blockManager.BlockList2D[r][c]->CheckCollision(ballList[i]))
						{
							blockManager.BlockList2D[r][c]->ResolveCollision(ballList[i]);
							blockManager.RemoveBlock(r, c);
						}
					}
				}
			}

			if (bPaddleValid && paddle->CheckCollision(ballList[i]))
			{
				paddle->ResolveCollision(ballList[i]);
			}
		}

		// 그리기
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			ballList[i]->Draw(renderer);
		}

		if (bBlockValid)
		{
			for (int r = 0; r < BlockRows; r++)
			{
				for (int c = 0; c < BlockCols; c++)
				{
					if (blockManager.BlockList2D[r][c])
					{
						blockManager.BlockList2D[r][c]->Draw(renderer);
					}
				}
			}
		}

		if (bPaddleValid)
		{
			paddle->Draw(renderer);
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Jungle Property Window");
		ImGui::Text("Hello Jungle World!");
		ImGui::Checkbox("Gravity", &bUseGravity);
		ImGui::Checkbox("Blocks", &bBlockValid);
		ImGui::Checkbox("Paddle", &bPaddleValid);
		ImGui::SetNextItemWidth(100);
		ImGui::SetNextItemWidth(100);
		if (ImGui::InputInt("Number of Balls", &ballCount, 1))
		{
			if (ballCount < 0)
			{
				ballCount = 0;
			}

			manager.ResizeBallList(ballCount, vertexBufferSphere, numVerticesSphere);
		}
		if (bBlockValid)
		{
			ImGui::Text("Number of Blocks: %d", UBlock::TotalNumBlocks);
		}
		ImGui::End();

		if (UBlock::TotalNumBlocks == 0)
		{
			ImGui::Begin("Clear!!!");
			if (ImGui::Button("Restart"))
			{
				blockManager.ReFillAllBlocks(vertexBufferCube, numVerticesCube);
			}
			ImGui::End();
		}

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

	// 공은 UPrimitiveManager 소멸자에서 해제
	// 블럭은 UBlockManager 소멸자에서 해제
	delete paddle;

	return 0;
}
