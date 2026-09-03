#pragma once

#pragma comment(lib, "d3d11")			
#pragma comment(lib, "d3dcompiler")		
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "gdi32.lib")

#include <d3d11.h>						
#include <d3dcompiler.h>
#include <d2d1.h>
#include <d2d1helper.h>
#include <dwrite.h>
#include <wincodec.h>
#include <vector>
#include <cmath>
#include <cstdlib>

#include "VertexSimple.h"
#include "Vector.h"
#include "enums.h"

struct FVertexBufferInfo
{
	ID3D11Buffer* vertexBuffer;
	UINT numVertucies;
};

class URenderer
{
	struct FConstants
	{
		FVector Offset;
		float Rotation;	// radians
		FVector Scale;	// (width, height, depth)
		float AspectRatio;
	};

public:
	static URenderer& GetInstance()
	{
		static URenderer instance;
		return instance;
	}

	FVector ShakeOffset;
	FVector ShakeVelocity;

	void Shake(FVector impulse)
	{
		ShakeVelocity = ShakeVelocity + impulse;
	}

	void ShakeRandom(float magnitude)
	{
		float radian = (float)rand() / RAND_MAX * acos(-1) * 2;
		ShakeVelocity = ShakeVelocity + FVector(cos(radian), sin(radian), 0) * magnitude;
	}

	void UpdateShake(float deltaTime)
	{
		constexpr float Stiffness = 80.0f;	// 원위치로 당기는 힘
		constexpr float Damping = 10.0f;		// 진동을 줄이는 힘

		FVector accel = ShakeOffset * -Stiffness - ShakeVelocity * Damping;
		ShakeVelocity = ShakeVelocity + accel * deltaTime;
		ShakeOffset = ShakeOffset + ShakeVelocity * deltaTime;

		if (ShakeOffset.LengthSquared() < 0.000001f && ShakeVelocity.LengthSquared() < 0.000001f)
		{
			ShakeOffset = FVector();
			ShakeVelocity = FVector();
		}
	}

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

	// Direct2D & WIC Management
	ID2D1Factory* D2DFactory = nullptr;
	ID2D1RenderTarget* D2DRenderTarget = nullptr;
	IDWriteFactory* DWriteFactory = nullptr;
	IWICImagingFactory* WICFactory = nullptr;

	// values
	D3D11_VIEWPORT ViewportInfo;
	FLOAT ClearColor[4] = { 0.025f, 0.025f, 0.025f, 1.0f };
	unsigned int Stride;
	float wAspectRatio;

public:
	void Create(HWND hWindow);
	void Release();

	void CreateDeviceAndSwapChain(HWND hWindow);
	void ReleaseDeviceAndSwapChain();

	void CreateFrameBuffer();
	void ReleaseFrameBuffer();

	void CreateRasterizerState();
	void ReleaseRasterizerState();

	void CreateShader();
	void ReleaseShader();

	void CreateConstantBuffer();
	void ReleaseConstantBuffer();

	void CreateVertexBufferInfos();
	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
	void ReleaseVertexBuffers();

	// Direct2D & Bitmap APIs
	bool CreateD2D();
	void ReleaseD2D();
	ID2D1Bitmap* LoadBitmapFromFile(const wchar_t* uri);
	void DrawBitmap(ID2D1Bitmap* bitmap, float left, float top, float width, float height, float opacity = 1.0f);
	void DrawWorldBitmap(ID2D1Bitmap* bitmap, const FVector& worldLocation, float rotation, const FVector& worldScale, float opacity = 1.0f, const D2D1_RECT_F* srcRect = nullptr);

	void Prepare();
	void PrepareShader();

	void UpdateConstant(FVector Offset, float Rotation, FVector Scale);
	void UpdateConstant(FVector Offset, FVector Scale);
	void RenderPrimitive(EPrimitive Primitive);
	void SwapBuffer();

private:
	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer);
	std::vector<FVertexBufferInfo> VertexBufferInfos;
};
