
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <directxmath.h>
#include <directxcolors.h>

#include "nvapi.h"
#include "nvapi_lite_stereo.h"


using namespace DirectX;

//--------------------------------------------------------------------------------------
// Structures
//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE                           g_hInst = nullptr;
HWND                                g_hWnd = nullptr;

ID3D11Device*                       g_pd3dDevice = nullptr;
ID3D11DeviceContext*                g_pImmediateContext = nullptr;
IDXGISwapChain*                     g_pSwapChain = nullptr;

ID3D11RenderTargetView*             g_pRenderTargetView = nullptr;
ID3D11Texture2D*                    g_pDepthStencil = nullptr;
ID3D11DepthStencilView*             g_pDepthStencilView = nullptr;

ID3D11VertexShader*                 g_pVertexShader = nullptr;
ID3D11PixelShader*                  g_pPixelShader = nullptr;
ID3D11InputLayout*                  g_pVertexLayout = nullptr;
ID3D11Buffer*                       g_pVertexBuffer = nullptr;
ID3D11Buffer*                       g_pIndexBuffer = nullptr;

ID3D11Buffer*                       g_pSharedCB = nullptr;

XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;

StereoHandle						g_StereoHandle;
UINT								g_ScreenWidth = 1280;
UINT								g_ScreenHeight = 720;


//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
HRESULT InitStereo();
HRESULT InitDevice();
HRESULT ActivateStereo();
void CleanupDevice();
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
void RenderFrame();


//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	if (FAILED(InitWindow(hInstance, nCmdShow)))
		return 0;

	if (FAILED(InitStereo()))
		return 0;

	if (FAILED(InitDevice()))
	{
		CleanupDevice();
		return 0;
	}

	if (FAILED(ActivateStereo()))
	{
		CleanupDevice();
		return 0;
	}

	// Main message loop
	MSG msg = { 0 };
	while (WM_QUIT != msg.message)
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			RenderFrame();
		}
	}

	CleanupDevice();

	return (int)msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow)
{
	// Register class
	WNDCLASSEX wcex;
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, (LPCTSTR)IDI_TUTORIAL1);
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = nullptr;
	wcex.lpszClassName = L"TutorialWindowClass";
	wcex.hIconSm = LoadIcon(wcex.hInstance, (LPCTSTR)IDI_TUTORIAL1);
	if (!RegisterClassEx(&wcex))
		return E_FAIL;

	// Create window
	g_hInst = hInstance;
	RECT rc = { 0, 0, g_ScreenWidth, g_ScreenHeight };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	g_hWnd = CreateWindow(L"TutorialWindowClass", L"Direct3D 11 Tutorial 7",
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance,
		nullptr);
	if (!g_hWnd)
		return E_FAIL;

	ShowWindow(g_hWnd, nCmdShow);

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Setup nvapi, and enable stereo by direct mode for the app.
// This must be called before the Device is created for Direct Mode to work.
//--------------------------------------------------------------------------------------
HRESULT InitStereo()
{
	NvAPI_Status status;

	status = NvAPI_Initialize();
	if (FAILED(status))
		return status;

	// The entire point is to show stereo.  
	// If it's not enabled in the control panel, let the user know.
	NvU8 stereoEnabled;
	status = NvAPI_Stereo_IsEnabled(&stereoEnabled);
	if (FAILED(status) || !stereoEnabled)
	{
		MessageBox(g_hWnd, L"3D Vision is not enabled. Enable it in the NVidia Control Panel.", L"Error", MB_OK);
		return status;
	}

	status = NvAPI_Stereo_SetDriverMode(NVAPI_STEREO_DRIVER_MODE_DIRECT);
	if (FAILED(status))
		return status;

	return status;
}


//--------------------------------------------------------------------------------------
// Activate stereo for the given device.
// This must be called after the device is created.
//--------------------------------------------------------------------------------------
HRESULT ActivateStereo()
{
	NvAPI_Status status;

	status = NvAPI_Stereo_CreateHandleFromIUnknown(g_pd3dDevice, &g_StereoHandle);
	if (FAILED(status))
		return status;

	return status;
}

//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
	HRESULT hr = S_OK;

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = g_ScreenWidth * 2;	// Swapchain needs to be 2x sized for direct stereo.
	sd.BufferDesc.Height = g_ScreenHeight;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 120;	// Needs to be 120Hz for 3D Vision 
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = g_hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;

	// Create the simple DX11, Device, SwapChain, and Context.
	hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, nullptr, 0,
		D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, nullptr, &g_pImmediateContext);
	if (FAILED(hr))
		return hr;

	// For DX11 3D, it's required that we run in exclusive full-screen mode, otherwise 3D
	// Vision will not activate.
	hr = g_pSwapChain->SetFullscreenState(TRUE, nullptr);
	if (FAILED(hr))
		return hr;

	// Create a render target view from the backbuffer
	//
	// Since this is derived from the backbuffer, it will also be 2x in width.
	ID3D11Texture2D* pBackBuffer = nullptr;
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pBackBuffer));
	if (FAILED(hr))
		return hr;
	hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_pRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr))
		return hr;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = g_ScreenWidth * 2;		// Direct stereo needs 2x size
	descDepth.Height = g_ScreenHeight;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D(&descDepth, nullptr, &g_pDepthStencil);
	if (FAILED(hr))
		return hr;

	// Create the depth stencil view
	//
	// This is not strictly necessary for our 3D, but is almost always used.
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_pd3dDevice->CreateDepthStencilView(g_pDepthStencil, &descDSV, &g_pDepthStencilView);
	if (FAILED(hr))
		return hr;

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// This viewport is 2x the screen width.  The documentation directly contradicts
	// this usage and suggests per-eye specific ViewPorts, but this works correctly.
	D3D11_VIEWPORT vp;
	vp.Width = (FLOAT)g_ScreenWidth * 2;		// Direct stereo needs the viewport 2x as well
	vp.Height = (FLOAT)g_ScreenHeight;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	g_pImmediateContext->RSSetViewports(1, &vp);



	return S_OK;
}


//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void CleanupDevice()
{
	if (g_pSwapChain) g_pSwapChain->SetFullscreenState(FALSE, nullptr);

	if (g_pImmediateContext) g_pImmediateContext->ClearState();

	if (g_pSharedCB) g_pSharedCB->Release();
	if (g_pVertexBuffer) g_pVertexBuffer->Release();
	if (g_pIndexBuffer) g_pIndexBuffer->Release();
	if (g_pVertexLayout) g_pVertexLayout->Release();

	if (g_pVertexShader) g_pVertexShader->Release();
	if (g_pPixelShader) g_pPixelShader->Release();
	if (g_pDepthStencil) g_pDepthStencil->Release();
	if (g_pDepthStencilView) g_pDepthStencilView->Release();
	if (g_pRenderTargetView) g_pRenderTargetView->Release();

	if (g_pSwapChain) g_pSwapChain->Release();
	if (g_pImmediateContext) g_pImmediateContext->Release();
	if (g_pd3dDevice) g_pd3dDevice->Release();

	if (g_StereoHandle) NvAPI_Stereo_DestroyHandle(g_StereoHandle);
}


//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		EndPaint(hWnd, &ps);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

		// Note that this tutorial does not handle resizing (WM_SIZE) requests,
		// so we created the window without the resize border.

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}


//--------------------------------------------------------------------------------------
// Render current image, eye independent.  
//--------------------------------------------------------------------------------------
void Render()
{
	//
	// Clear the back buffer
	//
	// Even though this uses the g_pRenderTargetView, it only affects half the backbuffer,
	// because we have set a specific eye.
	//
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, Colors::MidnightBlue);

	//
	// Clear the depth buffer to 1.0 (max depth)
	// 
	// Also done on a per-eye basis.
	//
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	//
	// Render the cube
	//
	// Projection matrix in g_pSharedCB determines eye view.
	//

}


//--------------------------------------------------------------------------------------
// Render a frame, both eyes.
//--------------------------------------------------------------------------------------
void RenderFrame()
{
	//
	// Rotate cube around the origin

	//
	// This now includes changing CBChangeOnResize each frame as well, because
	// we need to update the Projection matrix each frame, in case the user changes
	// the 3D settings.
	// The variable names are a bit misleading at present.
	//
	NvAPI_Status status;




	//
	// Drawing same object twice, once for each eye.
	// Eye specific setup is for the Projection matrix.
	// The _31 parameter is the X translation for the off center Projection.
	// The _41 parameter, I don't presently know what it is, but this
	// sequence works to handle both convergence and separation hot keys properly.
	//
	status = NvAPI_Stereo_SetActiveEye(g_StereoHandle, NVAPI_STEREO_EYE_LEFT);
	if (SUCCEEDED(status))
	{

		Render();
	}

	status = NvAPI_Stereo_SetActiveEye(g_StereoHandle, NVAPI_STEREO_EYE_RIGHT);
	if (SUCCEEDED(status))
	{


		Render();
	}

	//
	// Present our back buffer to our front buffer
	//
	// In stereo mode, the driver knows to use the 2x width buffer, and
	// present each eye in order.
	//
	g_pSwapChain->Present(0, 0);
}
