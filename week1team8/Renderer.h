#pragma once

#pragma comment(lib, "d3d11")			
#pragma comment(lib, "d3dcompiler")		

#include <d3d11.h>						
#include <d3dcompiler.h>

#include "VertexSimple.h"
#include "Vector.h"

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

	ID3D11Buffer* CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth);
	void ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer);

	void Prepare();
	void PrepareShader();

	void UpdateConstant(FVector Offset, FVector Scale);
	void RenderPrimitive(ID3D11Buffer* pBuffer, UINT numVertices);
	void SwapBuffer();
};
