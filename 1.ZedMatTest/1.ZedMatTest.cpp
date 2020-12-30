// 1.ZedMatTest.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <sl/Camera.hpp>
#include <opencv2/opencv.hpp>
using namespace sl;
struct ThreadData {
	sl::Camera zed;
	sl::Mat zed_image[2];
	std::mutex mtx;
	bool run;
	bool new_frame;
};
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
int main()
{
	//sl::Mat  mat = Mat(672, 376, sl::MAT_TYPE_32F_C1);
	sl::Mat  mat = sl::Mat(672, 376, sl::MAT_TYPE_8U_C3);
	//sl::Mat  mat = Mat(160, 90, sl::MAT_TYPE_32F_C1);


	printf("getWidth：\t%d \tReturns the width of the matrix.\n",
		mat.getWidth());

	printf("getHeight：\t%d \tReturns the height of the matrix. \n",
		mat.getHeight());

	printf("getChannels：\t%d \tReturns the number of values stored in one pixel. \n",
		mat.getChannels());

	printf("getStepBytes：\t%d \tReturns the memory step in Bytes (the Bytes size of one pixel row). \n",
		mat.getStepBytes(MEM_CPU));

	printf("getStep：\t%d \tReturns the memory step in number of elements (the number of values in one pixel row).\n",
		mat.getStep(MEM_CPU));

	printf("getPixelBytes：\t%d \tReturns the size in bytes of one pixel.\n",
		mat.getPixelBytes());

	printf("getWidthBytes：\t%d \tReturns the size in bytes of a row. \n",
		mat.getWidthBytes());

	int total = mat.getStep()*mat.getHeight();
	printf("%d total size\n", mat.getStep()*mat.getHeight());

	char* p = (char*)mat.getPtr<sl::uchar1>(MEM_CPU);
	printf("%u pointer\n", p);

	system("pause");
	cv::Mat cvmat = slMat2cvMat(mat);
	printf("MatChannels：\t%d \tReturns the Channels . \n",
		cvmat.channels());
	printf("MatSize：\t%d \tReturns the size . \n",
		cvmat.size());
	printf("MatStep：\t%d \tReturns the size in bytes of a row. \n",
		cvmat.step.buf[0]);
	system("pause");
	return 0;
}

