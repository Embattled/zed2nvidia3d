#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>

#include "d3dApp.hpp"
#include "winapp.hpp"

#pragma comment (lib, "d3d10.lib")



int main(int argc, char** argv)
{
	std::string title = "zed to d3d10";

	d3dApp d3dapp;
	d3dapp.createZed();
	WinApp  winapp(d3dapp.m_width, d3dapp.m_height, title);
	try
	{
		winapp.create();
		d3dapp.createD3d10(winapp.getHWDN());
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
