// ��� �߰����� ����(Include)�� ���� ������.
#pragma comment(lib, "user32")			
#pragma comment(lib, "d3d11")			
#pragma comment(lib, "d3dcompiler")		

#include <windows.h>					
#include <d3d11.h>						
#include <d3dcompiler.h>			

#include "ImGui/imgui.h"				
#include "ImGui/imgui_internal.h"		
#include "ImGui/imgui_impl_dx11.h"		
#include "ImGui/imgui_impl_win32.h"		

// FVertexSimple, triangle_vertices, cube_vertices, sphere_vertices
#include "Sphere.h"	

struct FVector
{
	float x, y, z;
	FVector(float _x = 0, float _y = 0, float _z = 0) : x(_x), y(_y), z(_z) {}

	float LengthSquared() const
	{
		return x * x + y * y + z * z;
	}

	float Length() const
	{
		return sqrt(LengthSquared());
	}

	void Normalize()
	{
		float length = Length();
		if (length > 0)
		{
			x /= length;
			y /= length;
			z /= length;
		}
	}

	float DotProduct(const FVector& other)
	{
		return x * other.x + y * other.y + z * other.z;
	}

	FVector operator+(const FVector& other) const
	{
		return FVector(x + other.x, y + other.y, z + other.z);
	}

	FVector operator-(const FVector& other) const
	{
		return FVector(x - other.x, y - other.y, z - other.z);
	}

	FVector operator*(float scalar) const
	{
		return FVector(x * scalar, y * scalar, z * scalar);
	}

	FVector& operator+=(const FVector& other)
	{
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	FVector& operator-=(const FVector& other)
	{
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}
};

class URenderer
{
	struct FConstants
	{
		FVector Offset;
		float Pad1;		// 16 byte pad
		FVector Scale;	// (width, height, depth)
		float Pad2;
	};

public:
	// CreateDeviceAndSwapChain
	ID3D11Device* Device = nullptr;
	ID3D11DeviceContext* DeviceContext = nullptr;
	IDXGISwapChain* SwapChain = nullptr;

	// CreateFrameBuffer
	ID3D11Texture2D* FrameBuffer = nullptr;
	ID3D11RenderTargetView* FrameBufferRTV = nullptr;

	// CreateRasterizerState
	ID3D11RasterizerState* RasterizerState = nullptr;

	// CreateShader
	ID3D11VertexShader* SimpleVertexShader = nullptr;
	ID3D11PixelShader* SimplePixelShader = nullptr;
	ID3D11InputLayout* SimpleInputLayout = nullptr;

	// CreateConstantBuffer
	ID3D11Buffer* ConstantBuffer = nullptr;

	// values
	D3D11_VIEWPORT ViewportInfo;
	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	unsigned int Stride;

public:
	void Create(HWND hWindow)
	{
		CreateDeviceAndSwapChain(hWindow);
		CreateFrameBuffer();
		CreateRasterizerState();
	}

	void Release()
	{
		ReleaseRasterizerState();
		DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
		ReleaseFrameBuffer();
		ReleaseDeviceAndSwapChain();
	}

	void CreateDeviceAndSwapChain(HWND hWindow)
	{
		D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };

		DXGI_SWAP_CHAIN_DESC swapchaindesc = {};
		swapchaindesc.BufferDesc.Width = 0;
		swapchaindesc.BufferDesc.Height = 0;
		swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapchaindesc.SampleDesc.Count = 1;
		swapchaindesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapchaindesc.BufferCount = 2;
		swapchaindesc.OutputWindow = hWindow;
		swapchaindesc.Windowed = TRUE;
		swapchaindesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

		D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE,
			nullptr, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG,
			featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION,
			&swapchaindesc, &SwapChain, &Device, nullptr, &DeviceContext);

		SwapChain->GetDesc(&swapchaindesc);
		ViewportInfo = { 0.0f, 0.0f,
			(float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height,
			0.0f, 1.0f };
	}

	void ReleaseDeviceAndSwapChain()
	{
		if (DeviceContext)
		{
			DeviceContext->Flush();
		}

		if (SwapChain)
		{
			SwapChain->Release();
			SwapChain = nullptr;
		}

		if (Device)
		{
			Device->Release();
			Device = nullptr;
		}

		if (DeviceContext)
		{
			DeviceContext->Release();
			DeviceContext = nullptr;
		}
	}

	void CreateFrameBuffer()
	{
		SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

		D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
		framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
		framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

		Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
	}

	void ReleaseFrameBuffer()
	{
		if (FrameBuffer)
		{
			FrameBuffer->Release();
			FrameBuffer = nullptr;
		}

		if (FrameBufferRTV)
		{
			FrameBufferRTV->Release();
			FrameBufferRTV = nullptr;
		}
	}

	void CreateRasterizerState()
	{
		D3D11_RASTERIZER_DESC rasterizerdesc = {};
		rasterizerdesc.FillMode = D3D11_FILL_SOLID;
		rasterizerdesc.CullMode = D3D11_CULL_BACK;

		Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
	}

	void ReleaseRasterizerState()
	{
		if (RasterizerState)
		{
			RasterizerState->Release();
			RasterizerState = nullptr;
		}
	}

	void CreateShader()
	{
		ID3DBlob* vertexshaderCSO;
		ID3DBlob* pixelshaderCSO;

		D3DCompileFromFile(
			L"ShaderW0.hlsl", nullptr, nullptr,
			"mainVS", "vs_5_0", 0, 0, &vertexshaderCSO, nullptr);

		Device->CreateVertexShader(
			vertexshaderCSO->GetBufferPointer(),
			vertexshaderCSO->GetBufferSize(), nullptr, &SimpleVertexShader);

		D3DCompileFromFile(
			L"ShaderW0.hlsl", nullptr, nullptr, "mainPS",
			"ps_5_0", 0, 0, &pixelshaderCSO, nullptr);

		Device->CreatePixelShader(
			pixelshaderCSO->GetBufferPointer(),
			pixelshaderCSO->GetBufferSize(), nullptr, &SimplePixelShader);

		D3D11_INPUT_ELEMENT_DESC layout[] =
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		Device->CreateInputLayout(
			layout, ARRAYSIZE(layout), vertexshaderCSO->GetBufferPointer(),
			vertexshaderCSO->GetBufferSize(), &SimpleInputLayout);

		Stride = sizeof(FVertexSimple);

		vertexshaderCSO->Release();
		pixelshaderCSO->Release();
	}

	void ReleaseShader()
	{
		if (SimpleInputLayout)
		{
			SimpleInputLayout->Release();
			SimpleInputLayout = nullptr;
		}

		if (SimplePixelShader)
		{
			SimplePixelShader->Release();
			SimplePixelShader = nullptr;
		}

		if (SimpleVertexShader)
		{
			SimpleVertexShader->Release();
			SimpleVertexShader = nullptr;
		}
	}

	void CreateConstantBuffer()
	{
		D3D11_BUFFER_DESC constantbufferdesc = {};
		constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
		constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
		constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
	}

	void ReleaseConstantBuffer()
	{
		if (ConstantBuffer)
		{
			ConstantBuffer->Release();
			ConstantBuffer = nullptr;
		}
	}

	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
	{
		D3D11_BUFFER_DESC vertexbufferdesc = {};
		vertexbufferdesc.ByteWidth = byteWidth;
		vertexbufferdesc.Usage = D3D11_USAGE_IMMUTABLE;
		vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertices };

		ID3D11Buffer* vertexBuffer;

		Device->CreateBuffer(&vertexbufferdesc, &vertexbufferSRD, &vertexBuffer);

		return vertexBuffer;
	}

	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
	{
		vertexBuffer->Release();
	}

	void Prepare()
	{
		DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

		DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		DeviceContext->RSSetViewports(1, &ViewportInfo);
		DeviceContext->RSSetState(RasterizerState);

		DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
		DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	}

	void PrepareShader()
	{
		DeviceContext->IASetInputLayout(SimpleInputLayout);

		DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
		if (ConstantBuffer)
		{
			DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
		}

		DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
	}

	void UpdateConstant(FVector Offset, FVector Scale)
	{
		if (ConstantBuffer)
		{
			D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

			DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
			FConstants* constants = (FConstants*)constantbufferMSR.pData;
			{
				constants->Offset = Offset;
				constants->Scale = Scale;
			}
			DeviceContext->Unmap(ConstantBuffer, 0);
		}
	}

	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices)
	{
		UINT offset = 0;
		DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
		DeviceContext->Draw(numVertices, 0);
	}

	void SwapBuffer()
	{
		SwapChain->Present(1, 0);
	}
};

// ȭ�� ��� (NDC)
const float leftBorder = -1.0f;
const float rightBorder = 1.0f;
const float topBorder = 1.0f;
const float bottomBorder = -1.0f;

// �߷�
const FVector G(0.0f, -9.8f, 0.0f);
bool bUseGravity = true;

// Ŭ���� �̸� �ܿ��� �����Ӱ� �����ϼ���.
class UPrimitive
{
public:
	// ������
	ID3D11Buffer* VertexBuffer;
	UINT NumVertices;

	virtual ~UPrimitive() {}
	virtual void Move(float t) = 0;							// t �ð����� �̵�
	virtual void Draw(URenderer& renderer) = 0;				// ȭ�鿡 �׸���
	virtual bool CheckCollision(UPrimitive* other) = 0;		// �浹 ����
	virtual void ResolveCollision(UPrimitive* other) = 0;	// �浹 �ذ� (�ӵ� ��ȭ, ��ħ �ذ�)
};

// Ŭ���� �̸���, �Ʒ� �ټ����� ���� �̸��� �������� �ʽ��ϴ�.
class UBall : public UPrimitive
{
public:
	FVector Location;			// ��ġ
	FVector Velocity;			// �ӵ�
	float Radius;				// ������
	float Mass;					// ����
	static int TotalNumBalls;	// �� ���� (������, �ı��� ����)

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

		// �ӵ� ��ȭ
		if (bUseGravity)
		{
			Velocity += G * deltaTime;
		}

		// ��ġ ��ȭ
		Location += Velocity * deltaTime;

		// ��-�� �浹 ���� �� �ذ�
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
		// ��ġ�� ũ�⸦ ��� ���ۿ� ������Ʈ
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

		// �� �� ������ �Ÿ��� �������� �պ��� ������ �浹
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

		// �浹 ���� ����
		FVector normal = diff;
		normal.Normalize();

		// ��� �ӵ�
		FVector v_rel = Velocity - otherBall->Velocity;

		// ��� �ӵ��� ������ �̷�� ����
		float v_rel_n = v_rel.DotProduct(normal);

		// �־����� �ִٸ� �浹 ó���� ���� ����
		if (v_rel_n >= 0)
		{
			return;
		}

		// �ݹ� ���
		float e = 1.0f;

		// ��ݷ�
		float j = -(1.0f + e) * v_rel_n / (1.0f / Mass + 1.0f / otherBall->Mass);

		// �ӵ� ��ȭ
		Velocity += normal * (j / Mass);
		otherBall->Velocity -= normal * (j / otherBall->Mass);

		// ��ħ �ذ�
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
	// �ݵ�� UBall�� �ƴ� UPrimitive�� �����Ͽ��� �ϸ� �ٲٸ� �ȵ˴ϴ�.
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

	// ������ ��ġ, �ӵ�, ũ�⸦ ���� �� ����
	UBall* CreateRandomBall(ID3D11Buffer* vertexBuffer, UINT numVertices)
	{
		// new �����ڸ� ����� ���� �ν��Ͻ�(Instance)�� ��� �����մϴ�.
		UBall* ball = new UBall();

		// ũ��
		float minRadius = 0.05f;
		float maxRadius = 0.10f;
		ball->Radius = (static_cast<float>(rand()) / RAND_MAX) * (maxRadius - minRadius) + minRadius;

		// ����
		float PI = acos(-1.0f);
		ball->Mass = 4.0f / 3.0f * PI * (ball->Radius * ball->Radius * ball->Radius);

		// ��ġ
		float locWidth = 0.5f;
		float locHeight = 0.5f;
		float locX = (static_cast<float>(rand()) / RAND_MAX) * locWidth - locWidth / 2;
		float locY = (static_cast<float>(rand()) / RAND_MAX) * locHeight - locHeight / 2;
		ball->Location = FVector(locX, locY);

		// �ӷ�
		float minSpeed = 1.0f;
		float maxSpeed = 5.0f;
		float speed = (static_cast<float>(rand()) / RAND_MAX) * (maxSpeed - minSpeed) + minSpeed;

		// ����
		float radian = (static_cast<float>(rand()) / RAND_MAX) * 2 * PI;
		FVector direction = FVector(cos(radian), sin(radian));

		// �ӵ�
		ball->Velocity = direction * speed;

		// ���ڷ� ���� ������ �����͸� ��� ������ �����մϴ�.
		ball->VertexBuffer = vertexBuffer;
		ball->NumVertices = numVertices;

		return ball;
	}

	void RemoveBall(int removeIndex)
	{
		int lastIndex = UBall::TotalNumBalls - 1;

		// delete �����ڸ� ����� ���� �ν��Ͻ��� ��� �Ҹ��ŵ�ϴ�.
		delete PrimitiveList[removeIndex];

		// ������ ��Ҹ� ������ �ڸ��� �����
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
				// �����ǰ� �ִ� ��ü ���� �� �ݵ�� ������(Random) ���� �Ҹ��ؾ� �մϴ�.
				int removeIndex = rand() % UBall::TotalNumBalls;

				RemoveBall(removeIndex);
			}
		}
		else if (newCount > UBall::TotalNumBalls)
		{
			Reserve(newCount);

			// ���� ����
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

		// �Ÿ��� �������� �պ��� ������ �浹
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

		// �浹 ���� ����
		FVector normal = diff;
		normal.Normalize();

		// ��� �ӵ�
		FVector v_rel = ball->Velocity * -1.0f;

		// ��� �ӵ��� ������ �̷�� ����
		float v_rel_n = v_rel.DotProduct(normal);

		// �־����� �ִٸ� �浹 ó���� ���� ����
		if (v_rel_n >= 0)
		{
			return;
		}

		// �ݹ� ���
		float e = 1.0f;

		// ��ݷ�
		float j = -(1.0f + e) * v_rel_n;

		// �ӵ� ��ȭ
		ball->Velocity -= normal * j;

		// ��ħ �ذ�
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
float UBlock::Height = 0.3f * 2 / BlockRows; // ���� * â ���� / ��
float UBlock::Width = 1.0f * 2 / BlockCols;	// ���� * â �ʺ� / ��
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

		// �Ÿ��� �������� �պ��� ������ �浹
		return diff.LengthSquared() < ball->Radius * ball->Radius;
	}

	// �е鿡 ���鿡 ���� ��� ���� ��ġ�� ���� ƨ��� ���� ����
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
				// -90���� ����, 0���� ����, 90���� ������

				ball->Velocity = FVector(sin(radian), cos(radian), 0.0f) * ball->Velocity.Length();

				// ��Ĩ �ذ�
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

				// ��ħ �ذ�
				ball->Location.y -= overlapY;
			}
		}
		else
		{
			ball->Velocity.x *= -1;

			// ��ħ �ذ�
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
	// ImGui �޽��� ó��
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

	// ������ �ʱ�ȭ
	URenderer renderer;
	renderer.Create(hWnd);
	renderer.CreateShader();
	renderer.CreateConstantBuffer();

	// ���� ���� ���
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	// ���ؽ� ����(Vertex Buffer)�� 1���� �����ϼ���.
	ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	// ImGui �ʱ�ȭ
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

	// �� �迭�� �����ϴ� Ŭ����
	UPrimitiveManager manager;
	UPrimitive**& ballList = manager.PrimitiveList;

	// ��� �迭�� �����ϴ� Ŭ����
	UBlockManager blockManager;
	blockManager.CreateBlocks(vertexBufferCube, numVerticesCube);

	// �е�
	UPaddle* paddle = CreatePaddle(vertexBufferCube, numVerticesCube);

	const int targetFPS = 144;
	const double targetFrameTime = 1000.0 / targetFPS;	// �� �������� ��ǥ �ð� (�и��� ����)

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

	// Main Loop (Quit Message�� ������ ������ �Ʒ� Loop�� ������ �����ϰ� ��)
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

		// �̵�
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			ballList[i]->Move(elapsedTime);
		}

		if (bPaddleValid)
		{
			paddle->Move(elapsedTime);
		}

		// �浹 �˻�
		for (int i = 0; i < UBall::TotalNumBalls; i++)
		{
			for (int j = i + 1; j < UBall::TotalNumBalls; j++)
			{
				// �� ��ü �浹 �߻�
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

		// �׸���
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

		ImGui::Render();										// �׸��� ��� �غ�	
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());	// �׸��� ��� ����

		renderer.SwapBuffer();

		do	// ������ ���
		{
			Sleep(0);
			QueryPerformanceCounter(&endTime);

			// �� �������� �ҿ�� �ð� ��� (�и��� ������ ��ȯ)
			elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;
		} while (elapsedTime < targetFrameTime);
	}

	ImGui_ImplDX11_Shutdown();	//ImGui ���ҽ� ����
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	// ������ ���ҽ� ����
	renderer.ReleaseVertexBuffer(vertexBufferCube);
	renderer.ReleaseVertexBuffer(vertexBufferSphere);
	renderer.ReleaseConstantBuffer();
	renderer.ReleaseShader();
	renderer.Release();

	// ���� UPrimitiveManager �Ҹ��ڿ��� ����
	// ����� UBlockManager �Ҹ��ڿ��� ����
	delete paddle;

	return 0;
}