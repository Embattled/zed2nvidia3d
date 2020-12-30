/*
// Sample demonstrating interoperability of OpenCV UMat with Direct X surface
// Base class for Direct X application
*/
#include <string>
#include <iostream>
#include <queue>

#include "opencv2/core.hpp"
#include "opencv2/core/directx.hpp"
#include "opencv2/core/ocl.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/videoio.hpp"

#include "winapp.hpp"
#include "mD3d10.hpp"
#define SAFE_RELEASE(p) if (p) { p->Release(); p = NULL; }


class D3DSample : public WinApp
{
public:

    D3DSample(int width, int height, std::string& window_name):
        WinApp(width, height, window_name)
    {
        m_shutdown          = false;
        m_demo_processing   = false;
    }

    ~D3DSample() {}
	int create(mD3d10 * md3d10)
	{
		int r=WinApp::create();
		md3d10->m_hWnd = m_hWnd; 
		rd3d10 = md3d10;
		return r;
	}
    int cleanup()
    {
        m_shutdown = true;
        return WinApp::cleanup();
    }
	
protected:
    virtual LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_CHAR:
            if (wParam == VK_SPACE)
            {
                m_demo_processing = !m_demo_processing;
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

    // do render at idle
    virtual int idle() { 
		return rd3d10->render();
		//return (*render)(); 
	}

protected:
    bool               m_shutdown;
    bool               m_demo_processing;
	mD3d10 * rd3d10;

};


static const char* keys =
{
    "{c camera | 0     | camera id  }"
    "{f file   |       | movie file name  }"
};
