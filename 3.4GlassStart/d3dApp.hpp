#pragma once

#include <windows.h>
#include <d3d10.h>
#include <nvapi.h>
#include <nvapi_lite_stereo.h>
#include "zed.hpp"

#define SAFE_RELEASE(p) if (p) { p->Release(); p = NULL; }

class d3dApp {
public:
	d3dApp() {}
	~d3dApp() {}

#define NVSTEREO_IMAGE_SIGNATURE 0x4433564e //NV3D
	typedef struct  _Nv_Stereo_Image_Header
	{
		unsigned int    dwSignature;
		unsigned int    dwWidth;
		unsigned int    dwHeight;
		unsigned int    dwBPP;
		unsigned int    dwFlags;
	} NVSTEREOIMAGEHEADER, *LPNVSTEREOIMAGEHEADER;
#define     SIH_SWAP_EYES               0x00000001
#define     SIH_SCALE_TO_FIT            0x00000002


	int create()
	{
		std::cout << "choice system mode" << std::endl;
		std::cout << "1.Zed grab to monitor show.(default)" << std::endl;
		std::cout << "2.Read image from disk and show at glass." << std::endl;
		std::cout << "3.Zed grab and show at glass." << std::endl;

		std::cin >> mode;
		switch (mode)
		{
		case 2:
			//return createImage();
			m_width = 1920;
			m_height = 1080;

			break;
		default:
			return createZed();

		}
		return 1;
	}
	int  createResource(HWND programHWND)
	{
		m_hWnd = programHWND;
		switch (mode)
		{
		case 2:
			if (createD3d10() != 0)
			{
				std::cout << "create d3d resource failure!" << std::endl;
				return 1;
			}
			if (createStereo() != 0)
			{
				std::cout << "create stereo failure!" << std::endl;
				return 1;
			}
			return 0;

		case 3:
			if (createD3d10() != 0)
			{
				std::cout << "create d3d resource failure!" << std::endl;
				return 1;
			}
			if (createStereo() != 0)
			{
				std::cout << "create stereo failure!" << std::endl;
				return 1;
			}
			return 0;
		default:
			if (createD3d10() != 0)
			{
				std::cout << "create d3d resource failure!" << std::endl;
				return 1;
			}
			return 0;
		}
		return 1;
	}
	int render()
	{
		switch (mode)
		{

		case 2:
			return renderGlass();
			break;
		case 3:
			zed.myCamaraGrab();
			return renderGlass();
		default:
			zed.myCamaraGrab();
			return renderMonitor();

		}
		return 1;
	}

	int createZed()
	{
		return zed.myModeChoiceAndCreateCamara(&m_eye_left, &m_eye_right, &m_eye_lr, &m_width, &m_height, mode);
	}
	int createD3d10()
	{

		// initialize DirectX
		HRESULT r;

		DXGI_SWAP_CHAIN_DESC scd;

		ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));

		scd.BufferCount = 1;                               // one back buffer
		scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;      // use 32-bit color
		//if (mode == 1)
			scd.BufferDesc.Width = m_width;                         // set the back buffer width
		//else
			//scd.BufferDesc.Width = m_width * 2;   //0000000000000000    // set the back buffer width

		scd.BufferDesc.Height = m_height;                        // set the back buffer height
		scd.BufferDesc.RefreshRate.Numerator = 120;	// Needs to be 120Hz for 3D Vision 
		scd.BufferDesc.RefreshRate.Denominator = 1;
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
			std::cout << "create device and swapchain error" << std::endl;
			return EXIT_FAILURE;
		}

		//set fullscreen
		r = m_pD3D10SwapChain->SetFullscreenState(TRUE, nullptr);
		if (FAILED(r))
			return r;


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

		//if (mode == 1)
			viewport.Width = m_width;
		//else
			//viewport.Width = m_width * 2;   //---00000000000000000
		viewport.Height = m_height;
		viewport.MinDepth = 0.0f;
		viewport.MaxDepth = 0.0f;

		m_pD3D10Dev->RSSetViewports(1, &viewport);


		//------------Two nomoral texture-------------------//
		D3D10_TEXTURE2D_DESC desc = { 0 };

		desc.MipLevels = 1;
		desc.ArraySize = 1;
		//desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.Usage = D3D10_USAGE_DYNAMIC;
		//desc.Usage            = D3D10_USAGE_STAGING;
		desc.CPUAccessFlags = D3D10_CPU_ACCESS_WRITE;
		desc.Height = m_height;
		desc.MiscFlags = 0;

		//if (mode == 1)
		{

			desc.Width = m_width;
			r = m_pD3D10Dev->CreateTexture2D(&desc, NULL, &m_pSurface);
			if (FAILED(r))
			{
				std::cerr << "Can't create textureL." << std::endl;
				return EXIT_FAILURE;
			}
			r = m_pD3D10Dev->CreateTexture2D(&desc, NULL, &m_pSurfaceR);
			if (FAILED(r))
			{
				std::cerr << "Can't create textureR." << std::endl;
				return EXIT_FAILURE;
			}
		}
		//else
		{
			desc.Width = m_width * 2;

			r = m_pD3D10Dev->CreateTexture2D(&desc, 0, &m_pStereoSurface);

			if (FAILED(r))
			{
				std::cerr << "Can't create textureLR." << std::endl;
				return EXIT_FAILURE;
			}

		}
		return EXIT_SUCCESS;

	}
	int createStereo()
	{
		NvAPI_Status status;
		status = NvAPI_Initialize();
		if (status != NVAPI_OK)
		{
			NvAPI_ShortString errorMessage;
			NvAPI_GetErrorMessage(status, errorMessage);
			MessageBoxA(NULL, errorMessage, "Unable to initialize NVAPI", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
		}
		else
		{
			// Check the Stereo availability
			NvU8 isStereoEnabled;
			status = NvAPI_Stereo_IsEnabled(&isStereoEnabled);
			// Stereo status report an error
			if (status != NVAPI_OK)
			{
				// GeForce Stereoscopic 3D driver is not installed on the system
				MessageBoxA(NULL, "Stereo is not available\nMake sure the stereo driver is installed correctly", "Stereo not available", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);

			}
			// Stereo is available but not enabled, let's enable it
			else if (NVAPI_OK == status && !isStereoEnabled)
			{
				MessageBoxA(NULL, "Stereo is available but not enabled\nLet's enable it", "Stereo not enabled", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
				status = NvAPI_Stereo_Enable();
			}

			status = NvAPI_Stereo_SetDriverMode(NVAPI_STEREO_DRIVER_MODE_DIRECT);
			if (FAILED(status))
			{
				MessageBoxA(NULL, "Stereo set driver mode failure", "Stereo not enabled", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
				return status;
			}

			//NvAPI_Stereo_CreateConfigurationProfileRegistryKey(NVAPI_STEREO_DEFAULT_REGISTRY_PROFILE);
		}

		status = NvAPI_Stereo_CreateHandleFromIUnknown(m_pD3D10Dev, &g_StereoHandle);
		if (NVAPI_OK != status)
		{
			MessageBoxA(NULL, "Couldn't create the StereoHandle", "NvAPI_Stereo_CreateHandleFromIUnknown failed", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
		}

		//if (NVAPI_OK != NvAPI_Stereo_GetEyeSeparation(g_StereoHandle, &g_EyeSeparation))
		//{
		//	MessageBoxA(NULL, "Couldn't get the hardware eye separation", "NvAPI_Stereo_GetEyeSeparation failed", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
		//}

		//if (NVAPI_OK != NvAPI_Stereo_Activate(g_StereoHandle))
		//{
		//	MessageBoxA(NULL, "Couldn't activate stereo", "NvAPI_Stereo_Activate failed", MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
		//}

		//------create stereo texture and copy stereo header
		//------Stereo Parameter Write and Merge
		unsigned int pixelSize = 4;
		D3D10_SUBRESOURCE_DATA sysData;
		sysData.pSysMem = new unsigned char[pixelSize*m_width * 2 * (m_height + 1)];
		sysData.SysMemPitch = 2 * m_width*pixelSize;

		LPNVSTEREOIMAGEHEADER pStereoImageHeader = (LPNVSTEREOIMAGEHEADER)(((unsigned char *)sysData.pSysMem) + (sysData.SysMemPitch * m_height));
		pStereoImageHeader->dwSignature = NVSTEREO_IMAGE_SIGNATURE;
		pStereoImageHeader->dwBPP = pixelSize * 8;

		// We'll put left image on the left and right image on the right, so no need for swap
		pStereoImageHeader->dwFlags = 0; //SIH_SWAP_EYES;
		pStereoImageHeader->dwWidth = m_width * 2;
		pStereoImageHeader->dwHeight = m_height;

		D3D10_TEXTURE2D_DESC desc;
		memset(&desc, 0, sizeof(D3D10_TEXTURE2D_DESC));
		desc.Width = m_width * 2;
		desc.Height = m_height + 1;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.Usage = D3D10_USAGE_DEFAULT;
		desc.BindFlags = D3D10_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		HRESULT r;
		r = m_pD3D10Dev->CreateTexture2D(&desc, &sysData, &m_pstereoParTexture);
		delete sysData.pSysMem;
		if (FAILED(r))
		{
			std::cout << "stereo parameter texture create failure!" << std::endl;
			return 1;
		}
		// free the initial data
		//----------------------------------------



		return 0;
	}

	int cvmatToTexture()
	{
		HRESULT r;

		//if (mode == 1)
		{

			UINT subResource = ::D3D10CalcSubresource(0, 0, 1);
			UINT subResourceR = ::D3D10CalcSubresource(0, 0, 1);

			D3D10_MAPPED_TEXTURE2D mappedTex;
			D3D10_MAPPED_TEXTURE2D mappedTexR;
			r = m_pSurface->Map(subResource, D3D10_MAP_WRITE_DISCARD, 0, &mappedTex);
			if (FAILED(r))
			{
				return r;
			}
			r = m_pSurfaceR->Map(subResourceR, D3D10_MAP_WRITE_DISCARD, 0, &mappedTexR);
			if (FAILED(r))
			{
				return r;
			}
			cv::Mat m(m_height, m_width, CV_8UC4, mappedTex.pData, (int)mappedTex.RowPitch);
			m_eye_left.copyTo(m);
			cv::Mat mR(m_height, m_width, CV_8UC4, mappedTexR.pData, (int)mappedTexR.RowPitch);
			m_eye_right.copyTo(mR);
			m_pSurface->Unmap(subResource);
			m_pSurfaceR->Unmap(subResourceR);

		}
		//else
		{

			UINT subResourceLR = ::D3D10CalcSubresource(0, 0, 1);
			D3D10_MAPPED_TEXTURE2D mappedTexLR;
			r = m_pStereoSurface->Map(subResourceLR, D3D10_MAP_WRITE_DISCARD, 0, &mappedTexLR);
			if (FAILED(r))
			{
				return r;
			}
			cv::Mat m(m_height, m_width * 2, CV_8UC4, mappedTexLR.pData, (int)mappedTexLR.RowPitch);
			m_eye_lr.copyTo(m);
			m_pStereoSurface->Unmap(subResourceLR);
		}
		return EXIT_SUCCESS;
	} // get_surface()
	int renderMonitor()
	{
		try
		{
			HRESULT r;
			r = cvmatToTexture();
			if (FAILED(r))
			{
				return EXIT_FAILURE;
			}

			//if (nowEye == 0)
			m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pSurface);
			r = m_pD3D10SwapChain->Present(1, 0);
			//else
			m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pSurfaceR);
			r = m_pD3D10SwapChain->Present(1, 0);

			nowEye = 1 - nowEye;
			//r = m_pD3D10SwapChain->Present(1, 0);
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
	}
	int renderGlass()
	{
		
		try
		{
		HRESULT r;
		r = cvmatToTexture();
		if (FAILED(r))
		{
		return EXIT_FAILURE;
		}


		//Copy image data to sysTex
		D3D10_BOX stereoDataBox;
		stereoDataBox.front = 0;
		stereoDataBox.back = 1;
		stereoDataBox.top = 0;
		stereoDataBox.bottom = m_height;
		stereoDataBox.left = 0;
		stereoDataBox.right = m_width*2;
		m_pD3D10Dev->CopySubresourceRegion(m_pstereoParTexture, 0, 0, 0, 0, m_pStereoSurface,0, &stereoDataBox);

		// Copy the sysTex to the back buffer
		D3D10_BOX stereoSrcBox;
		stereoSrcBox.front = 0;
		stereoSrcBox.back = 1;
		stereoSrcBox.top = 0;
		stereoSrcBox.bottom = m_height;
		stereoSrcBox.left = 0;
		stereoSrcBox.right = m_width;

		m_pD3D10Dev->CopySubresourceRegion(m_pBackBuffer, 0, 0, 0, 0, m_pstereoParTexture, 0, &stereoSrcBox);

		// free the sys texture
		//------------------------------------------------------



		{
		//m_pD3D10Dev->CopySubresourceRegion(m_pBackBuffer, 0, 0, 0, 0, m_pSurface, 0, NULL);
		//m_pD3D10Dev->CopySubresourceRegion(m_pBackBuffer, 0, m_width, 0, 0, m_pSurfaceR, 0, NULL);
		//m_pD3D10Dev->CopySubresourceRegion(m_pBackBuffer, 0, 0, 0, 0,m_pStereoSurface, 0, NULL);
		//m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pStereoSurface);
		}

		r = m_pD3D10SwapChain->Present(0, 0);

		if (FAILED(r))
		{
		std::cout << "Present Error!" << std::endl;
		return EXIT_FAILURE;
		}
		} // try
		catch (const cv::Exception& e)
		{
		std::cerr << "Exception: " << e.what() << std::endl;
		return 10;
		}
		//


		
		//HRESULT r;
		//r = cvmatToTexture();
		//if (FAILED(r))
		//{
		//	return EXIT_FAILURE;
		//}
		////m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pStereoSurface);

		//NvAPI_Status status;
		//status = NvAPI_Stereo_SetActiveEye(g_StereoHandle, NVAPI_STEREO_EYE_LEFT);
		//if (SUCCEEDED(status))
		//{
		//	
		//	//Render()
		//	m_pD3D10Dev->CopySubresourceRegion(m_pBackBuffer, 0, 0, 0, 0, m_pSurface, 0, NULL);
		//}

		//status = NvAPI_Stereo_SetActiveEye(g_StereoHandle, NVAPI_STEREO_EYE_RIGHT);
		//if (SUCCEEDED(status))
		//{
		//	m_pD3D10Dev->CopySubresourceRegion(m_pBackBuffer, 0, m_width, 0, 0, m_pSurfaceR, 0, NULL);

		//	//Render();
		//}
		////m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pStereoSurface);
		//m_pD3D10SwapChain->Present(0, 0);
		//return EXIT_SUCCESS;
		
	}

	int cleanup(void)
	{
		SAFE_RELEASE(m_pSurface);
		SAFE_RELEASE(m_pBackBuffer);
		SAFE_RELEASE(m_pD3D10SwapChain);
		SAFE_RELEASE(m_pRenderTarget);
		SAFE_RELEASE(m_pD3D10Dev);
		return EXIT_SUCCESS;
	} // cleanup()

	int mode;
	int     m_width;
	int     m_height;
	HWND        m_hWnd;

	StereoHandle g_StereoHandle = 0;
	float        g_EyeSeparation = 0;
	float        g_Separation = 0;
	float        g_Convergence = 0;


	myZed   zed;
	cv::Mat        m_eye_left;
	cv::Mat        m_eye_right;
	cv::Mat        m_eye_lr;
	int			nowEye = 0;


	ID3D10Device*           m_pD3D10Dev;
	IDXGISwapChain*         m_pD3D10SwapChain;
	ID3D10Texture2D*        m_pBackBuffer;
	ID3D10Texture2D*        m_pSurface;
	ID3D10Texture2D*        m_pSurfaceR;
	ID3D10Texture2D*        m_pstereoParTexture;
	ID3D10Texture2D*        m_pStereoSurface;
	//ID3D10ShaderResourceView*       m_pStereoSurfaceSRV;

	ID3D10RenderTargetView* m_pRenderTarget;


};