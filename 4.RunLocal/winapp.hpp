#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#define WINCLASS "WinAppWnd"
#define SAFE_RELEASE(p) if (p) { p->Release(); p = NULL; }

class d3dApp;
class WinApp
{
public:
	WinApp(int width, int height, std::string& window_name)
	{
		//m_width = width * 2;
		m_width = width;//-0000000000000000
		m_height = height;
		m_window_name = window_name;
		m_hInstance = ::GetModuleHandle(NULL);
		m_hWnd = 0;
		m_shutdown = false;
	}
	WinApp()
	{
		m_hInstance = ::GetModuleHandle(NULL);
		m_hWnd = 0;
		m_shutdown = false;
	}

	~WinApp() {}

	int create(int mode)
	{
		if (m_height == 0 || m_width == 0)
		{
			std::cout << "program height or weight = 0!" << std::endl;
			return 1;

		}


		WNDCLASSEX wcex;

		wcex.cbSize = sizeof(WNDCLASSEX);
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = &WinApp::StaticWndProc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = 0;
		wcex.hInstance = m_hInstance;
		wcex.hIcon = LoadIcon(0, IDI_APPLICATION);
		wcex.hCursor = LoadCursor(0, IDC_ARROW);
		wcex.hbrBackground = 0;
		wcex.lpszMenuName = 0L;
		wcex.lpszClassName = WINCLASS;
		wcex.hIconSm = 0;

		ATOM wc = ::RegisterClassEx(&wcex);
		if (!wc)
			return -1;
		RECT rc = { 0, 0, m_width, m_height };
		if (!::AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false))
			return -1;

		m_hWnd = ::CreateWindow(
			(LPCTSTR)wc, m_window_name.c_str(),
			WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
			rc.right - rc.left, rc.bottom - rc.top,
			NULL, NULL, m_hInstance, (void*)this);

		if (!m_hWnd)
			return -1;

		::ShowWindow(m_hWnd, SW_SHOW);
		::UpdateWindow(m_hWnd);
		::SetFocus(m_hWnd);

		return 0;
	} // create()

	int run()
	{
		MSG msg;

		::ZeroMemory(&msg, sizeof(msg));

		while (msg.message != WM_QUIT)
		{
			if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
			{
				::TranslateMessage(&msg);
				::DispatchMessage(&msg);
			}
			else
			{
				if (idle() != 0)
				{
					break;
				}
			}
		}

		return static_cast<int>(msg.wParam);
	} // run()
	int cleanup()
	{
		m_shutdown = true;
		::DestroyWindow(m_hWnd);
		::UnregisterClass(WINCLASS, m_hInstance);
		return 0;
	} // cleanup()

	HWND getHWDN()
	{
		return m_hWnd;
	}

	void setRenderFun(d3dApp* d3dapp)
	{
		this->d3dapp = d3dapp;
	}
protected:

	static LRESULT CALLBACK StaticWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		WinApp* pWnd;

		if (message == WM_NCCREATE)
		{
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			pWnd = static_cast<WinApp*>(pCreateStruct->lpCreateParams);
			::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
		}

		pWnd = GetObjectFromWindow(hWnd);

		if (pWnd)
			return pWnd->WndProc(hWnd, message, wParam, lParam);
		else
			return ::DefWindowProc(hWnd, message, wParam, lParam);
	} // StaticWndProc()

	inline static WinApp* GetObjectFromWindow(HWND hWnd) { return (WinApp*)::GetWindowLongPtr(hWnd, GWLP_USERDATA); }

	// actual wnd message handling
	LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		{
			switch (message)
			{
			case WM_CHAR:
				if (wParam == VK_SPACE)
				{
					m_shutdown = !m_shutdown;
					return EXIT_SUCCESS;
				}
				else if (wParam == VK_ESCAPE)
				{
					return cleanup();
				}
				break;
			case WM_CLOSE:
				return cleanup();

			case WM_DESTROY:
				::PostQuitMessage(0);
				return EXIT_SUCCESS;
			}

			return ::DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
	// idle processing
	int idle()
	{
		if (d3dapp != NULL)
			return d3dapp->render();
		else
			return 0;
	}

	d3dApp*		d3dapp = NULL;
	HINSTANCE   m_hInstance;
	HWND        m_hWnd;
	int         m_width;
	int         m_height;
	std::string m_window_name;
	bool        m_shutdown;

};