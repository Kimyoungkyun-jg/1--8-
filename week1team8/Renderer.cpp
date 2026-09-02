#include <DirectXMath.h>

#include "Renderer.h"
#include "Sphere.h"	

void URenderer::Create(HWND hWindow)
{
	CreateDeviceAndSwapChain(hWindow);
	CreateFrameBuffer();
	CreateRasterizerState();
	CreateD2D();
}

void URenderer::Release()
{
	ReleaseD2D();
	ReleaseRasterizerState();
	DeviceContext->OMSetRenderTargets(0, nullptr, nullptr);
	ReleaseFrameBuffer();
	ReleaseDeviceAndSwapChain();
}

void URenderer::CreateDeviceAndSwapChain(HWND hWindow)
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

	wAspectRatio = (float)swapchaindesc.BufferDesc.Width / (float)swapchaindesc.BufferDesc.Height;
}

void URenderer::ReleaseDeviceAndSwapChain()
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

void URenderer::CreateFrameBuffer()
{
	SwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&FrameBuffer);

	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {};
	framebufferRTVdesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	Device->CreateRenderTargetView(FrameBuffer, &framebufferRTVdesc, &FrameBufferRTV);
}

void URenderer::ReleaseFrameBuffer()
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

void URenderer::CreateRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerdesc = {};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID;
	rasterizerdesc.CullMode = D3D11_CULL_BACK;

	Device->CreateRasterizerState(&rasterizerdesc, &RasterizerState);
}

void URenderer::ReleaseRasterizerState()
{
	if (RasterizerState)
	{
		RasterizerState->Release();
		RasterizerState = nullptr;
	}
}

void URenderer::CreateShader()
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

void URenderer::ReleaseShader()
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

void URenderer::CreateConstantBuffer()
{
	D3D11_BUFFER_DESC constantbufferdesc = {};
	constantbufferdesc.ByteWidth = sizeof(FConstants) + 0xf & 0xfffffff0;
	constantbufferdesc.Usage = D3D11_USAGE_DYNAMIC;
	constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	constantbufferdesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	Device->CreateBuffer(&constantbufferdesc, nullptr, &ConstantBuffer);
}

void URenderer::ReleaseConstantBuffer()
{
	if (ConstantBuffer)
	{
		ConstantBuffer->Release();
		ConstantBuffer = nullptr;
	}
}

void URenderer::CreateVertexBufferInfos()
{
	UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
	UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);

	ID3D11Buffer* vertexBufferCube = CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
	ID3D11Buffer* vertexBufferSphere = CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	VertexBufferInfos.push_back({ vertexBufferSphere , numVerticesSphere });
	VertexBufferInfos.push_back({ vertexBufferCube , numVerticesCube });
}


ID3D11Buffer* URenderer::CreateVertexBuffer(FVertexSimple* vertices, UINT byteWidth)
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

void URenderer::ReleaseVertexBuffers()
{
	for (FVertexBufferInfo& vi : VertexBufferInfos)
	{
		ReleaseVertexBuffer(vi.vertexBuffer);
	}
}

void URenderer::ReleaseVertexBuffer(ID3D11Buffer* vertexBuffer)
{
	vertexBuffer->Release();
}

void URenderer::Prepare()
{
	DeviceContext->ClearRenderTargetView(FrameBufferRTV, ClearColor);

	DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	DeviceContext->RSSetViewports(1, &ViewportInfo);
	DeviceContext->RSSetState(RasterizerState);

	DeviceContext->OMSetRenderTargets(1, &FrameBufferRTV, nullptr);
	DeviceContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
}

void URenderer::PrepareShader()
{
	DeviceContext->IASetInputLayout(SimpleInputLayout);

	DeviceContext->VSSetShader(SimpleVertexShader, nullptr, 0);
	if (ConstantBuffer)
	{
		DeviceContext->VSSetConstantBuffers(0, 1, &ConstantBuffer);
	}

	DeviceContext->PSSetShader(SimplePixelShader, nullptr, 0);
}

void URenderer::UpdateConstant(FVector Offset, float Rotation, FVector Scale)
{
	if (ConstantBuffer)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		DeviceContext->Map(ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
		FConstants* constants = (FConstants*)constantbufferMSR.pData;
		{
			constants->Offset = Offset;
			constants->Rotation = Rotation; // Radians
			constants->Scale = Scale;
			constants->AspectRatio = wAspectRatio;
		}
		DeviceContext->Unmap(ConstantBuffer, 0);
	}
}

void URenderer::UpdateConstant(FVector Offset, FVector Scale)
{
	//Scale.y *= ViewportInfo.Width / ViewportInfo.Height;
	UpdateConstant(Offset, 0.0f, Scale);
}

void URenderer::RenderPrimitive(EPrimitive Primitive)
{
	UINT offset = 0;

	ID3D11Buffer* pBuffer = nullptr;
	UINT numVertices;
	if (Primitive == EPrimitive::Circle)
	{ 
		pBuffer = VertexBufferInfos[0].vertexBuffer;
		numVertices = VertexBufferInfos[0].numVertucies;
	}
	else if (Primitive == EPrimitive::Rectangle)
	{
		pBuffer = VertexBufferInfos[1].vertexBuffer;
		numVertices = VertexBufferInfos[1].numVertucies;
	}

	DeviceContext->IASetVertexBuffers(0, 1, &pBuffer, &Stride, &offset);
	DeviceContext->Draw(numVertices, 0);
}

void URenderer::SwapBuffer()
{
	SwapChain->Present(1, 0);
}

bool URenderer::CreateD2D()
{
	if (!SwapChain) return false;

	HRESULT hr;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &D2DFactory);
	if (FAILED(hr)) return false;

	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&DWriteFactory));
	if (FAILED(hr)) return false;

	IDXGISurface* surface = nullptr;
	hr = SwapChain->GetBuffer(0, IID_PPV_ARGS(&surface));
	if (FAILED(hr)) return false;

	D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
		D2D1_RENDER_TARGET_TYPE_DEFAULT,
		D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	hr = D2DFactory->CreateDxgiSurfaceRenderTarget(surface, &props, &D2DRenderTarget);
	surface->Release();
	if (FAILED(hr)) return false;

	CoInitialize(nullptr);
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&WICFactory)
	);
	if (FAILED(hr)) return false;

	return true;
}

void URenderer::ReleaseD2D()
{
	if (D2DRenderTarget) { D2DRenderTarget->Release(); D2DRenderTarget = nullptr; }
	if (DWriteFactory) { DWriteFactory->Release(); DWriteFactory = nullptr; }
	if (D2DFactory) { D2DFactory->Release(); D2DFactory = nullptr; }
	if (WICFactory) { WICFactory->Release(); WICFactory = nullptr; }
}

ID2D1Bitmap* URenderer::LoadBitmapFromFile(const wchar_t* uri)
{
	if (!WICFactory || !D2DRenderTarget) return nullptr;

	IWICBitmapDecoder* decoder = nullptr;
	IWICBitmapFrameDecode* frame = nullptr;
	IWICFormatConverter* converter = nullptr;
	ID2D1Bitmap* bitmap = nullptr;

	if (FAILED(WICFactory->CreateDecoderFromFilename(uri, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, &decoder)))
	{
		return nullptr;
	}

	if (FAILED(decoder->GetFrame(0, &frame)))
	{
		decoder->Release();
		return nullptr;
	}

	if (FAILED(WICFactory->CreateFormatConverter(&converter)))
	{
		frame->Release();
		decoder->Release();
		return nullptr;
	}

	converter->Initialize(
		frame,
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		nullptr,
		0.0f,
		WICBitmapPaletteTypeMedianCut
	);

	D2DRenderTarget->CreateBitmapFromWicBitmap(converter, nullptr, &bitmap);

	converter->Release();
	frame->Release();
	decoder->Release();

	return bitmap;
}

void URenderer::DrawBitmap(ID2D1Bitmap* bitmap, float left, float top, float width, float height, float opacity)
{
	if (!D2DRenderTarget || !bitmap) return;

	D2D1_RECT_F destRect = D2D1::RectF(left, top, left + width, top + height);
	D2DRenderTarget->BeginDraw();
	D2DRenderTarget->DrawBitmap(bitmap, &destRect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR);
	D2DRenderTarget->EndDraw();
}

void URenderer::DrawWorldBitmap(ID2D1Bitmap* bitmap, const FVector& worldLocation, float rotation, const FVector& worldScale, float opacity, const D2D1_RECT_F* srcRect)
{
	if (!D2DRenderTarget || !bitmap) return;

	float aspect = (ViewportInfo.Height > 0.0f) ? (ViewportInfo.Width / ViewportInfo.Height) : (16.0f / 9.0f);
	float screenX = (worldLocation.x / aspect + 1.0f) * 0.5f * ViewportInfo.Width;
	float screenY = (1.0f - worldLocation.y) * 0.5f * ViewportInfo.Height;

	float screenW = (worldScale.x / aspect) * 0.5f * ViewportInfo.Width;
	float screenH = (worldScale.y) * 0.5f * ViewportInfo.Height;

	D2D1_RECT_F destRect = D2D1::RectF(-screenW * 0.5f, -screenH * 0.5f, screenW * 0.5f, screenH * 0.5f);

	D2DRenderTarget->BeginDraw();
	D2D1::Matrix3x2F oldTransform;
	D2DRenderTarget->GetTransform(&oldTransform);

	D2D1::Matrix3x2F transform = D2D1::Matrix3x2F::Rotation(-rotation * (180.0f / 3.14159265f), D2D1::Point2F(0, 0))
		* D2D1::Matrix3x2F::Translation(screenX, screenY);

	D2DRenderTarget->SetTransform(transform);
	D2DRenderTarget->DrawBitmap(bitmap, &destRect, opacity, D2D1_BITMAP_INTERPOLATION_MODE_LINEAR, srcRect);
	D2DRenderTarget->SetTransform(oldTransform);
	D2DRenderTarget->EndDraw();
}
