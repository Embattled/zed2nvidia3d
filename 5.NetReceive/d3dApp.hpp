#pragma once

#include <windows.h>
#include <d3d10.h>
#include <opencv2/opencv.hpp>
#include <nvapi.h>
#include <nvapi_lite_stereo.h>
#include "WinSock2.h"
#include <thread>
#pragma comment(lib,"ws2_32.lib")

#define SAFE_RELEASE(p) if (p) { p->Release(); p = NULL; }

//#define BLOCKSIZE 57600
#define BLOCKSIZE 15792


struct recvbuf//包格式
{
	char buf[BLOCKSIZE];//存放数据的变量
	int flag;//标志
};

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
		//m_width = 1920;
		m_width = 1280;
		//m_height = 1080;
		m_height = 720;


		m_eye_lr = cv::Mat(94 * 2, 168 * 4, CV_8UC4);
		return 1;
	}
	int  createResource(HWND programHWND)
	{
		m_hWnd = programHWND;

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
		if (createLan() != 0)
		{
			std::cout << "create socket failure!" << std::endl;
			return 1;

		}
		return 0;

	}
	int render()
	{
		return renderGlass();
		//return renderMonitor();
		//return netReceive();
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
		desc.Width = m_width * 2;

		r = m_pD3D10Dev->CreateTexture2D(&desc, 0, &m_pStereoSurface);

		if (FAILED(r))
		{
			std::cerr << "Can't create textureLR." << std::endl;
			return EXIT_FAILURE;
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
	void threadReceive()
	{
		while (true)
		{
			if (SOCKET_ERROR == recvfrom(sockClient, (char*)&recPackage, sizeof(recvbuf), 0, (LPSOCKADDR)&addrRec, &nLen))
			{
				std::cout << "收包错误！" << std::endl;
			}
			memcpy(dataPoint + (recPackage.flag)*BLOCKSIZE, recPackage.buf, BLOCKSIZE);
		}
	}
	int createLan()
	{

		//char ip[20];
		//printf("输入本机ipIP\n");
		//scanf("%s", ip);
		//printf("本机IP:%s\n", ip);
		WSADATA wsaData;
		int port = 5099;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			printf("加载套接字失败\n");
			return 1;
		}

		addrRec.sin_family = AF_INET;
		addrRec.sin_port = htons(port);
		//addrRec.sin_addr.s_addr = inet_addr(ip);
		addrRec.sin_addr.s_addr = htonl(INADDR_ANY);
		sockClient = socket(AF_INET, SOCK_DGRAM, 0);
		if (sockClient == SOCKET_ERROR)
		{
			printf("套接字创建失败\n");
			return 1;
		}
		if (bind(sockClient, (LPSOCKADDR)&addrRec, sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
		{
			printf("套接字绑定失败:%d\n", WSAGetLastError);
			return 1;
		}
		else
		{
			printf("套接字绑定成功，开始监听");

		}

		SOCKADDR_IN addrSend;
		nLen = sizeof(SOCKADDR);
		int nRecvBuf = 1280 * 720 * 4 * 2 * 30;//接收缓存
		setsockopt(sockClient, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));
		dataPoint = (char*)m_eye_lr.datastart;

		std::thread recThread(&d3dApp::threadReceive, this);
		recThread.detach();
	}
	int netReceive()
	{


		//int count = 0;
		//for (int i = 0 ; i < 128; i++)
		//{
		//	if (SOCKET_ERROR != recvfrom(sockClient, (char*)&recPackage, sizeof(recvbuf), 0, (LPSOCKADDR)&addrRec, &nLen))
		//	{
		//		//std::cout << "收到第" << i << "数据包,flag是"<<recPackage.flag << std::endl;
		//	}
		//	//count++;

		//	memcpy(dataPoint + (recPackage.flag)*BLOCKSIZE , recPackage.buf, BLOCKSIZE);
		//	//dataPoint += BLOCKSIZE;
		//}
		//if (count != 129)
		//{
		//	std::cout << "Frame rec failure" << std::endl;
		//}
		//else


			cv::imshow("receiver", m_eye_lr);

			if (cv::waitKey(5) == 27)
			{
				cleanup();
				exit(0);
			}


		return 0;
	}
	int cvmatToTexture()
	{
		HRESULT r;

		UINT subResourceLR = ::D3D10CalcSubresource(0, 0, 1);
		D3D10_MAPPED_TEXTURE2D mappedTexLR;
		r = m_pStereoSurface->Map(subResourceLR, D3D10_MAP_WRITE_DISCARD, 0, &mappedTexLR);
		if (FAILED(r))
		{
			return r;
		}
		cv::Mat m(m_height, m_width * 2, CV_8UC4, mappedTexLR.pData, (int)mappedTexLR.RowPitch);
		cv::resize(m_eye_lr,m,cv::Size(m_width*2,m_height));
		//m_eye_lr.copyTo(m);
		m_pStereoSurface->Unmap(subResourceLR);
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
			m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pStereoSurface);
			r = m_pD3D10SwapChain->Present(1, 0);
			/*
			//if (nowEye == 0)
			m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pSurface);
			r = m_pD3D10SwapChain->Present(1, 0);
			//else
			m_pD3D10Dev->CopyResource(m_pBackBuffer, m_pSurfaceR);
			r = m_pD3D10SwapChain->Present(1, 0);
			nowEye = 1 - nowEye;*/

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
			stereoDataBox.right = m_width * 2;
			//m_pD3D10Dev->CopySubresourceRegion(m_pstereoParTexture, 0, 0, 0, 0, m_pStereoSurface, 0, &stereoDataBox);
			m_pD3D10Dev->CopySubresourceRegion(m_pstereoParTexture, 0, 0, 0, 0, m_pStereoSurface, 0, NULL);

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

	cv::Mat        m_eye_lr;
	double timer;
	double fps;
	char fps_string[10];
	SOCKADDR_IN addrRec;
	int nLen;
	char* dataPoint;
	SOCKET sockClient;
	recvbuf recPackage;


	ID3D10Device*           m_pD3D10Dev;
	IDXGISwapChain*         m_pD3D10SwapChain;
	ID3D10Texture2D*        m_pBackBuffer;
	ID3D10Texture2D*        m_pstereoParTexture;
	ID3D10Texture2D*        m_pStereoSurface;
	//ID3D10ShaderResourceView*       m_pStereoSurfaceSRV;

	ID3D10RenderTargetView* m_pRenderTarget;


};