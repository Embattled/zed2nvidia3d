#pragma once

#include<opencv2/opencv.hpp>
#include<sl/Camera.hpp>



class myZed {
public:
	myZed() {}
	~myZed() {}
	int createCamera(cv::Mat *lrMat, int *width, int *height)
	{

		//init_parameters.camera_resolution = sl::RESOLUTION_HD720;
		init_parameters.camera_resolution = sl::RESOLUTION_VGA;
		init_parameters.depth_mode = sl::DEPTH_MODE_NONE;
		//init_parameters.camera_fps = 15;
		init_parameters.camera_fps = 60;


		sl::ERROR_CODE err_ = zed.open(init_parameters);
		if (err_ != sl::SUCCESS) {
			std::cout << "ERROR: " << sl::toString(err_) << std::endl;
			zed.close();
			return -1;
		}

		*width = zed.getResolution().width;
		*height = zed.getResolution().height;

		zed_image.alloc(zed.getResolution().width*2, zed.getResolution().height, sl::MAT_TYPE_8U_C4, sl::MEM_CPU);
		zed_image_bgra.alloc(zed.getResolution().width*2, zed.getResolution().height, sl::MAT_TYPE_8U_C4, sl::MEM_CPU);

		cv_image_bgra= slMat2cvMat(zed_image_bgra);
		*lrMat = slMat2cvMat(zed_image);
		cv_image_rgba = lrMat;


		runtime_parameters.enable_depth = false;

		return 0;
	}
	bool cameraGrab() {
		if (zed.grab(runtime_parameters) == sl::SUCCESS)
		{

			zed.retrieveImage(zed_image_bgra, (sl::VIEW_SIDE_BY_SIDE), sl::MEM_CPU);

			cv::cvtColor(cv_image_bgra, *cv_image_rgba, cv::COLOR_BGRA2RGBA);

			return true;
		}
		else return false;
	}

private:

	sl::Camera zed;
	sl::Mat zed_image;
	sl::Mat zed_image_bgra;
	cv::Mat cv_image_bgra;
	cv::Mat *cv_image_rgba;
	sl::InitParameters init_parameters;
	sl::RuntimeParameters runtime_parameters;


	static	cv::Mat slMat2cvMat(sl::Mat& input) {
		// Mapping between MAT_TYPE and CV_TYPE
		int cv_type = -1;
		switch (input.getDataType()) {
		case sl::MAT_TYPE_32F_C1: cv_type = CV_32FC1; break;
		case sl::MAT_TYPE_32F_C2: cv_type = CV_32FC2; break;
		case sl::MAT_TYPE_32F_C3: cv_type = CV_32FC3; break;
		case sl::MAT_TYPE_32F_C4: cv_type = CV_32FC4; break;
		case sl::MAT_TYPE_8U_C1: cv_type = CV_8UC1; break;
		case sl::MAT_TYPE_8U_C2: cv_type = CV_8UC2; break;
		case sl::MAT_TYPE_8U_C3: cv_type = CV_8UC3; break;
		case sl::MAT_TYPE_8U_C4: cv_type = CV_8UC4; break;
		default: break;
		}
		// Since cv::Mat data requires a uchar* pointer, we get the uchar1 pointer from sl::Mat (getPtr<T>())
		// cv::Mat and sl::Mat will share a single memory structure
		return cv::Mat(input.getHeight(), input.getWidth(), cv_type, input.getPtr<sl::uchar1>(sl::MEM_CPU));
	}

};

