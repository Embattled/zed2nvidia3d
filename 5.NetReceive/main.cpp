#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>

#include "d3dApp.hpp"
#include "winapp.hpp"
#include <stdio.h>

#pragma comment (lib, "d3d10.lib")



int main(int argc, char** argv)
{
	std::string title = "zed to nvidia 3d vision";

	d3dApp d3dapp;
	d3dapp.create();

	WinApp  winapp(d3dapp.m_width, d3dapp.m_height, title);
	try
	{
		if (winapp.create()!=0)
		{
			std::cout << "create windows error!" << std::endl;
			return 1;
		}
		if (d3dapp.createResource(winapp.getHWDN())!= 0)
		{
			std::cout << "create direct3D resource error!" << std::endl;
			return 1;
		}
		
		winapp.setRenderFun(&d3dapp);
		return winapp.run();
	}

	catch (const cv::Exception& e)
	{
		std::cerr << "Exception: " << e.what() << std::endl;
		return 10;
	}

	catch (...)
	{
		std::cerr << "FATAL ERROR: Unknown exception" << std::endl;
		return 11;
	}

}
