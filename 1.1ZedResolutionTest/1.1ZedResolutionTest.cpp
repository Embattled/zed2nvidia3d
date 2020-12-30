// 1.1ZedResolutionTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <sl/Camera.hpp>
#include <opencv2/opencv.hpp>


int main()
{
	sl::InitParameters init_parameters;
	init_parameters.depth_mode = sl::DEPTH_MODE_NONE;
	init_parameters.camera_resolution = sl::RESOLUTION_HD720;

	//Receive not need open camera
	sl::Camera zed;
	
		
	sl::ERROR_CODE err_ = zed.open(init_parameters);
	if (err_ != sl::SUCCESS) {
	std::cout << "ERROR: " << sl::toString(err_) << std::endl;
	zed.close();
	return -1;
	}
	
	int zedWidth = zed.getResolution().width;
	int zedHeight = zed.getResolution().height;
	std::cout << "Width" << zedWidth << std::endl;
	std::cout << "Height" << zedHeight << std::endl;
	std::cout << "CUDA" << zed.getCUDAContext() << std::endl;
	system("pause");

}

