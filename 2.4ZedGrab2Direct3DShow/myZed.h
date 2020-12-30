#pragma once
#include<opencv2/opencv.hpp>
#include<sl/Camera.hpp>

class myZed {
public:
	myZed() {}
	~myZed() {}
	int myModeChoiceAndCreateCamara(cv::Mat *leftMat, cv::Mat *rightMat, int *width, int *height);
	bool myCamaraGrab();

	int getNowEye();
	sl::InitParameters getInit_parameters;
private:

	sl::Camera zed;
	sl::Mat zed_image[2];
	sl::InitParameters init_parameters;
	sl::RuntimeParameters runtime_parameters;
	int fpsMode;
	int fpsNum;
	sl::VIEW imageMode;
	int nowEye = 1;//left eye
};

cv::Mat slMat2cvMat(sl::Mat& input);
