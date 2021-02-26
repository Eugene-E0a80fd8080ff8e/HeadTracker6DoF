#include "stdafx.h"
#include "after_stdafx.h"

#include <memory.h>

using namespace cv;

const double camera_K[9] = { -1, 0.0, -1, 0.0, -1, -1, 0.0, 0.0, 1.0 };
double camera_D[5] = { 0, 0, 0, 0, 0 };


Camera::Camera() {

}

int Camera::getHeightByWidth(int width)
{
	switch (width)
	{
	case 320: return 240; break;
	case 640: return 480; break;
	case 800: return 600; break;
	case 1024: return 768; break;
	case 1280: return 768; break;
	case 1920: return 1080; break;
	default: return -1;
	}
}

bool Camera::init(int camIdx, int resolutionWidth, int resolutionHeight)
{
	
	cap.open(camIdx);
	if (!cap.isOpened()) {
		std::cerr << "Error: Can not access the camera" << endl;
		exit(13);
	}

	requestNewResolution(resolutionWidth, resolutionHeight);
	/*
	//cap.set(cv::CAP_PROP_FOURCC, 'MJPG');
	//cap.set(cv::CAP_PROP_FOURCC, 'X264');
	cap.set(CAP_PROP_FRAME_WIDTH, resolutionWidth);
	cap.set(CAP_PROP_FRAME_HEIGHT, resolutionHeight);
	//cap.set(cv::CAP_PROP_FPS,30);
	//cap.set(cv::CAP_PROP_FOURCC, MKFOURCC('M', 'J', 'P', 'G'));
	cap.set(CAP_PROP_BUFFERSIZE, 1);
	//cap.set(CAP_PROP_BUFFERSIZE, 0);
	*/
	return true;
}

void Camera::requestNewResolution(int width,int height)
{
	if(height == -1)
		height = getHeightByWidth(width);

	cap.set(CAP_PROP_FRAME_WIDTH, width);
	cap.set(CAP_PROP_FRAME_HEIGHT, height);
	cap.set(CAP_PROP_BUFFERSIZE, 1);
}

const cv::Mat& Camera::getCameraK()
{
	if (frame.rows != camK_latest_resY || frame.cols != camK_latest_resX)
	{
		camK_latest_resY = frame.rows;
		camK_latest_resX = frame.cols;

		double K[9];
		memcpy(K, camera_K, 9 * sizeof(double));
		K[0] = camK_latest_resX;
		K[2] = camK_latest_resX / 2;
		K[4] = camK_latest_resX;
		K[5] = camK_latest_resY / 2;

		mat_camera_K = Mat(3, 3, CV_64FC1, K).clone();
	}
	return mat_camera_K;
}

const cv::Mat& Camera::getCameraD()
{
	if (!mat_camera_D_init)
	{
		mat_camera_D = Mat(5, 1, CV_64FC1, camera_D).clone();
	}
	return mat_camera_D;

}

void Camera::capture()
{
	cap >> frame;
}

cv::Mat Camera::getFrame()
{
	return frame;
}

