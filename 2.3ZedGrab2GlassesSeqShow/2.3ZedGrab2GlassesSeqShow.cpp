// 2.3ZedGrab2GlassesSeqShow.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#ifndef WINVER
#define WINVER         0x0500
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS 0x0500
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT   0x0600
#endif


#include <sl/Camera.hpp>
#include <opencv2/opencv.hpp>
#include "stdio.h"
#include "string"
using namespace sl;
cv::Mat slMat2cvMat(sl::Mat& input);
int main()
{




	sl::Camera zed;
	sl::Mat zed_image[2];
	cv::Mat cv_image[2];
	sl::InitParameters init_parameters;
	double fps;
	char fps_string[10];
	int resolutionMode;
	int fpsMode;
	int fpsSelect[4] = { 15,30,60,100 };

	printf("Choice your resolution! Input number.\n");
	printf("1.RESOLUTION_HD2K\t2208*1242,\tavailable framerates: 15 fps.\n");
	printf("2.RESOLUTION_HD1080\t1920*1080,\tavailable framerates: 15, 30 fps.\n");
	printf("3.RESOLUTION_HD720\t1280*720,\tavailable framerates: 15, 30, 60 fps.\n");
	printf("4.RESOLUTION_VGA\t672*376,\tavailable framerates: 15, 30, 60, 100 fps.\n");
	scanf("%d", &resolutionMode);
	switch (resolutionMode)
	{
	case 1:init_parameters.camera_resolution = sl::RESOLUTION_HD2K; break;
	case 2:init_parameters.camera_resolution = sl::RESOLUTION_HD1080; break;
	case 3:init_parameters.camera_resolution = sl::RESOLUTION_HD720; break;
	default:init_parameters.camera_resolution = sl::RESOLUTION_VGA; break;
	}

	init_parameters.depth_mode = sl::DEPTH_MODE_NONE;
	printf("Choice your fps!Input number!\n");
	for (int i = 0; i < 4; i++)
	{
		printf("%d : %d\tis ", i + 1, fpsSelect[i]);
		printf(resolutionMode>i?"available\n":"Not available\n");
	}
	scanf("%d", &fpsMode);
	switch (fpsMode)
	{
	case 1:init_parameters.camera_fps = 15; break;
	case 2:init_parameters.camera_fps = 30; break;
	case 3:init_parameters.camera_fps = 60; break;
	case 4:init_parameters.camera_fps = 100; break;
	default:100;
	}



	sl::ERROR_CODE err_ = zed.open(init_parameters);
	if (err_ != sl::SUCCESS) {
		std::cout << "ERROR: " << sl::toString(err_) << std::endl;
		zed.close();
		return -1;
	}

	sl::RuntimeParameters runtime_parameters;
	runtime_parameters.enable_depth = false;


	for (int i = 0; i < 2; i++)
	{
		//zed_image[i].alloc(zed.getResolution(), MAT_TYPE_8U_C4, MEM_CPU);
		zed_image[i].alloc(zed.getResolution(), MAT_TYPE_8U_C1, MEM_CPU);
		cv_image[i] = slMat2cvMat(zed_image[i]);
	}

	cv::Mat* outputMat;
	double t = 0;
	double hz = 0;
	cv::namedWindow("img", CV_WINDOW_NORMAL);
	cv::setWindowProperty("img", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
			int nowEye = 0;
	while (true)
	{
		//t = (double)cv::getTickCount();

		if (zed.grab(runtime_parameters) == sl::SUCCESS) {

			//for (int nowEye = 0; nowEye < 2; nowEye++)
			nowEye = 1 - nowEye;
			{
				zed.retrieveImage(zed_image[1-nowEye], (sl::VIEW)(3-nowEye));
				//zed.retrieveImage(zed_image[1], sl::VIEW_RIGHT);

				outputMat = &cv_image[nowEye];
				//outputMat = &MatNocolor[0];

				hz = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
				t = cv::getTickCount();

				fps = 1.0 / hz;
				sprintf(fps_string, "%.2f", fps);
				std::string fpsString("FPS:");
				fpsString += fps_string;

				std::cout << fpsString<< std::endl;

				cv::putText(
					*outputMat,
					fpsString,
					cv::Point(0, 15),
					cv::FONT_HERSHEY_SIMPLEX,
					0.5,
					cv::Scalar(255, 255, 255));

				cv::imshow("img", *outputMat);

				if (cv::waitKey(5)== 27)
					exit(0);	
			}
			//system("cls");
		}
	}
	return 0;
}

cv::Mat slMat2cvMat(sl::Mat& input) {
	// Mapping between MAT_TYPE and CV_TYPE
	int cv_type = -1;
	switch (input.getDataType()) {
	case MAT_TYPE_32F_C1: cv_type = CV_32FC1; break;
	case MAT_TYPE_32F_C2: cv_type = CV_32FC2; break;
	case MAT_TYPE_32F_C3: cv_type = CV_32FC3; break;
	case MAT_TYPE_32F_C4: cv_type = CV_32FC4; break;
	case MAT_TYPE_8U_C1: cv_type = CV_8UC1; break;
	case MAT_TYPE_8U_C2: cv_type = CV_8UC2; break;
	case MAT_TYPE_8U_C3: cv_type = CV_8UC3; break;
	case MAT_TYPE_8U_C4: cv_type = CV_8UC4; break;
	default: break;
	}

	// Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
	// cv::Mat and sl::Mat will share a single memory structure
	return cv::Mat(input.getHeight(), input.getWidth(), cv_type, input.getPtr<sl::uchar1>(MEM_CPU));
}