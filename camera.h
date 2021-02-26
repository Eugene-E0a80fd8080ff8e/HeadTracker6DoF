#pragma once


class Camera {

	Camera(const Camera&) = delete;
	Camera& operator=(Camera const&) = delete;

	cv::VideoCapture cap;
	cv::Mat frame;

	int camK_latest_resX = -1, camK_latest_resY = -1;
	bool mat_camera_D_init = false;
	cv::Mat mat_camera_K, mat_camera_D;

	int getHeightByWidth(int width);

public:

	Camera();
	bool init(int camIdx, int camResolutionWidth, int camResolutionHeight = -1);

	void requestNewResolution(int width,int height = -1); // for already initialized camera

	const cv::Mat & getCameraK();
	const cv::Mat & getCameraD();

	int width() const;
	int height() const;

	void capture();
	cv::Mat getFrame();

};