#pragma once

#include <windows.h>
#include <d3d10.h>

#include "zed.hpp"

#define SAFE_RELEASE(p) if (p) { p->Release(); p = NULL; }

class d3dApp {
public:
	d3dApp() {}
	~d3dApp(){}

	void createZed()
	{
		zed.myModeChoiceAndCreateCamara(&m_eye_left, &m_eye_right, &m_width, &m_height);
	}
	int createD3d10(HWND programHWND)
	{
		m_hWnd = programHWND;
		// initialize DirectX
		HRESULT r;

		DXGI_SWAP_CHAIN_DESC scd;

		ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

		scd.BufferCount = 1;                               // one back buffer
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // use 32-bit color
		scd.BufferDesc.Width = m_width;                         // set the back buffer width
		scd.BufferDesc.Height = m_height;                        // set the back buffer height
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // how swap chain is to be used
		scd.OutputWindow = m_hWnd;                          // the window to be used
		scd.SampleDesc.Count = 1;                               // how many multisamples
		scd.Windowed = TRUE;                            // windowed/full-screen mode
		scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH; // allow full-screen switching

		r = ::D3D10CreateDeviceAndSwapChain(
			NULL,
			D3D10_DRIVER_TYPE_HARDWARE,
			NULL,
			0,
			D3D10_SDK_VERSION,
			&scd,
			&m_pD3D10SwapChain,
			&m_pD3D10Dev);
		if (FAILED(r))
		{
			return EXIT_FAILURE;
		}


		r = m_pD3D10SwapChain->GetBuffer(0, __uuidof(ID3D10Texture2D), (LPVOID*)&m_pBackBuffer);
		if (FAILED(r))
		{
			return EXIT_FAILURE;
		}


		r = m_pD3D10Dev->CreateRenderTargetView(m_pBackBuffer, NULL, &m_pRenderTarget);
		if (FAILED(r))
		{
			return EXIT_FAILURE;
		}


		m_pD3D10Dev->OMSetRenderTargets(1, &m_pRenderTarget, NULL);

		D3D10_VIEWPORT viewport;
		ZeroMemory(&viewport, sizeof(D3D10_VIEWPORT));

		viewport.Width = m_width;
		viewport.Height = m_height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 0.0f;

		m_pD3D10Dev->RSSetViewports(1, &viewport);

		D3D10_TEXTURE2D_DESC desc = { 0 };

		desc.Width = m_width;
		desc.Height = m_height;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.Usage = D3D10_USAGE_DYNAMIC;
		//desc.Usage            = D3D10_USAGE_STAGING;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;

		r = m_pD3D10Dev->CreateTexture2D(&desc, NULL, &m_pSurface);
		if (FAILED(r))
		{
			std::cerr << "Can't create texture with input image" << std::endl;
			return EXIT_FAILURE;
		}


		return EXIT_SUCCESS;
	}// createD3d10()

	int get_surface()
	{
		HRESULT r;

		//if(nowEye==0)
		if (!zed.myCamaraGrab())
			return EXIT_FAILURE;

		UINT subResource = ::D3D10CalcSubresource(0, 0, 1);

		D3D10_MAPPED_TEXTURE2D mappedTex;
		r = m_pSurface->Map(subResource, D3D10_MAP_WRITE_DISCARD, 0, &mappedTex);
		if (FAILED(r))
		{
			return r;
		}

		cv::Mat m(m_height, m_width, CV_8UC4, mappedTex.pData, (int)mappedTex.RowPitch);
		// copy video frame data to surface

		//m_eye_frame[zed.getNowEye()].copyTo(m);
		if (nowEye == 1)
		{
			//m_eye_right.convertTo(m_eye_right, cv::COLOR_RGBA2BGRA);
			m_eye_right.copyTo(m);
		}
		else
			//m_eye_left.convertTo(m_eye_left, cv::COLOR_RGBA2BGRA);
			m_eye_left.copyTo(m);


		m_pSurface->Unmap(subResource);

		nowEye = 1 - nowEye;
		return EXIT_SUCCESS;
	} // get_surface()

	// process and render media data
	int render()
	{
		try
		{
			//if (m_shutdown)
			//    return EXIT_SUCCESS;

			// capture user input once

			HRESULT r;


			r = get_surface();
			if (FAILED(r))
			{
				return EXIT_FAILURE;
			}



			// traditional DX render pipeline:
			//   BitBlt surface to backBuffer and flip backBuffer to frontBuffer
			//m_pD3D10Dev->CopyResource(m_pBackBuffer, pSurface);
			m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pSurface);

			// present the back buffer contents to the display
			// switch the back buffer and the front buffer
			r = m_pD3D10SwapChain->Present(1, 0);
			if (FAILED(r))
			{
				return EXIT_FAILURE;
			}
		} // try

		catch (const cv::Exception& e)
		{
			std::cerr << "Exception: " << e.what() << std::endl;
			return 10;
		}

		return EXIT_SUCCESS;
	} // render()


	int cleanup(void)
	{
		SAFE_RELEASE(m_pSurface);
		SAFE_RELEASE(m_pBackBuffer);
		SAFE_RELEASE(m_pD3D10SwapChain);
		SAFE_RELEASE(m_pRenderTarget);
		SAFE_RELEASE(m_pD3D10Dev);
		return EXIT_SUCCESS;
	} // cleanup()


	int     m_width;
	int     m_height;
	HWND        m_hWnd;

	myZed   zed;
	cv::Mat        m_eye_left;
	cv::Mat        m_eye_right;
	int			nowEye = 0;


	ID3D10Device*           m_pD3D10Dev;
	IDXGISwapChain*         m_pD3D10SwapChain;
	ID3D10Texture2D*        m_pBackBuffer;
	ID3D10Texture2D*        m_pSurface;
	ID3D10RenderTargetView* m_pRenderTarget;
};