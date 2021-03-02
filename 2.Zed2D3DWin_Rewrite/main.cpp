/*
// A sample program demonstrating interoperability of OpenCV cv::UMat with Direct X surface
// At first, the data obtained from video file or camera and placed onto Direct X surface,
// following mapping of this Direct X surface to OpenCV cv::UMat and call cv::Blur function.
// The result is mapped back to Direct X surface and rendered through Direct X API.
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d10.h>

#include "d3dsample.hpp"
#include "mD3d10.hpp"

#pragma comment (lib, "d3d10.lib")



int main(int argc, char** argv)
{
    std::string title = "zed to d3d10";

	mD3d10 md3d10;
	md3d10.createZed();
	D3DSample  mwindows(md3d10.m_width, md3d10.m_height, title);
	try
	{
		mwindows.create(&md3d10);
		md3d10.createD3d10();
		return mwindows.run();
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
