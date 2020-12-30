// 2.ZedGrab2OpenCVShow.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
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
	sl::InitParameters init_parameters;
	double fps;
	char fps_string[10];

	printf("Choice your resolution! Input number.\n");
	printf("1.RESOLUTION_HD2K\t2208*1242,\tavailable framerates: 15 fps.\n");
	printf("2.RESOLUTION_HD1080\t1920*1080,\tavailable framerates: 15, 30 fps.\n");
	printf("3.RESOLUTION_HD720\t1280*720,\tavailable framerates: 15, 30, 60 fps.\n");
	printf("4.RESOLUTION_VGA\t672*376,\tavailable framerates: 15, 30, 60, 100 fps.\n");
	int resolutionMode;
	scanf("%d", &resolutionMode);
	switch (resolutionMode)
	{
		case 1:init_parameters.camera_resolution = sl::RESOLUTION_HD2K; break;
		case 2:init_parameters.camera_resolution = sl::RESOLUTION_HD1080; break;
		case 3:init_parameters.camera_resolution = sl::RESOLUTION_HD720; break;
		default:init_parameters.camera_resolution = sl::RESOLUTION_VGA; break;
	}

	init_parameters.depth_mode = sl::DEPTH_MODE_NONE;
	init_parameters.camera_fps = 30;

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
		zed_image[i].alloc(zed.getResolution(),MAT_TYPE_8U_C4, MEM_CPU);
	}

	sl::Mat matLinkZed(zed.getResolution().width*2,zed.getResolution().height,MAT_TYPE_8U_C4,sl::MEM_CPU);
	cv::Mat matLinkCV = slMat2cvMat(matLinkZed);
	cv::Mat leftMat = slMat2cvMat(zed_image[0]);
	cv::Mat rightMat = slMat2cvMat(zed_image[1]);
	cv::Mat MatNocolor[2];

	cv::Mat* outputMat;
	double t = 0;
	double hz = 0;
	while (true)
	{
		t = (double)cv::getTickCount();

		if (zed.grab(runtime_parameters) == sl::SUCCESS) {

			zed.retrieveImage(zed_image[0], sl::VIEW_LEFT);
			zed.retrieveImage(zed_image[1], sl::VIEW_RIGHT);

			//printf("before size %d", leftMat.dataend - leftMat.datastart);
/*
			cv::cvtColor(leftMat, MatNocolor[0], cv::COLOR_RGBA2GRAY);
			cv::cvtColor(rightMat, MatNocolor[1], cv::COLOR_RGBA2GRAY);*/
			//printf("before size %d", MatNocolor[0].dataend - MatNocolor[0].datastart);

			//输出彩色图像
			//cv::imshow("leftvedio", leftMat);
			//cv::imshow("rightvedio", rightMat);

			//输出黑白图像
			//cv::imshow("leftvedio", MatNocolor[0]);
			//cv::imshow("rightvedio", MatNocolor[1]);

			//输出合成图像
			cv::hconcat(leftMat, rightMat, matLinkCV);
			//cv::imshow("Link", matLinkCV);

			//输出合成黑白
			//cv::cvtColor(matLinkCV, MatNocolor[0], cv::COLOR_RGBA2GRAY);
			//cv::imshow("Link", MatNocolor[0]);
			//printf(" size %d \n", MatNocolor[0].dataend - MatNocolor[0].datastart);

			outputMat = &matLinkCV;
			//outputMat = &MatNocolor[0];

			hz = ((double)cv::getTickCount() - t) / cv::getTickFrequency();
			t = cv::getTickCount();

			fps = 1.0 / hz;
			sprintf(fps_string, "%.2f", fps);
			std::string fpsString("FPS:");
			fpsString += fps_string;

			std::cout << fpsString << std::endl;
			
			cv::putText(
				*outputMat,
				fpsString,
				cv::Point(0,15),
				cv::FONT_HERSHEY_SIMPLEX,
				0.5,
				cv::Scalar(255, 255, 255));

			cv::imshow("Link", *outputMat);
			//system("cls");
		}
			cv::waitKey(30);
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