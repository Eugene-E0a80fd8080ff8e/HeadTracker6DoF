
#include "stdafx.h"
#include "after_stdafx.h"

//	#define CVUI_IMPLEMENTATION
#include "cvui.h"
#include "argagg.hpp"

using namespace cv;

AprilTagsTriplets aprilTagTriplets;

shared_ptr<Camera> camera;

const char* const  windowTitle = "Web Cam Head Tracking";

argagg::parser argparser{ {
	{ "help", {"-h", "--help"},
	  "shows this help message", 0},
	{ "camera", {"-c", "--camera"},
	  "Use camera # (default: 0)", 1},
	{ "width", {"--width"},
	  "Request camera's width (default: 1280)", 1},
	{ "height", { "--height"},
	  "Request camera's height (default: 768)", 1},
	/*{ "fps", {"--fps"},
	  "Request camera's frame frequency (default: 30)", 1},*/

	{ "head", { "--head","--process-head"} ,
		"Detect and transmit location and rotation of user's head" , 0},
	{ "apriltags", { "--process-apriltags"} ,
		"Detect and transmit locations of Apriltags. As of now, Apriltags are used to represent hands" , 0},
	{ "facial", { "--process-facial"} ,
		"Detect and transmit facial metrics" , 0},
	/*{ "headtracker", { "--headtracker"} ,
		"Compatibility mode for HeadTracker.cs plugin" , 0},*/

		/*
	{ "outputip", { "--output-to-ip"} ,
		"Configures to output UDP packets to specified IP address (default is the local address: 127.0.0.1)" , 1},
	{ "outputport", { "--output-to-port"} ,
		"Configures to output UDP packets to specified IP port (default: 62731)" , 1},
		*/
  } };

int main(int argc, char* argv[])
{
	int selectedCamera = 0;
	int CAM_WIDTH = 1280;
	int CAM_HEIGHT = 768;

	argagg::parser_results args;
	try {
		args = argparser.parse(argc, argv);
	}
	catch (const std::exception& e) {
		argagg::fmt_ostream fmt(std::cerr);
		fmt << argparser;
		return EXIT_FAILURE;
	}

	if (args["help"]) {
		std::cerr << argparser;
		return EXIT_SUCCESS;
	}

	if (args["camera"]) {
		selectedCamera = args["camera"];
	}
	if (args["width"]) {
		CAM_WIDTH = args["width"];
	}
	if (args["height"]) {
		CAM_HEIGHT = args["height"];
	}

	camera = make_shared<Camera>();
	camera->init(selectedCamera, CAM_WIDTH);

	{
		aprilTagTriplets.init(camera);
	}

	std::cout << "Using camera #" << selectedCamera << endl;

	cv::namedWindow(windowTitle, cv::WINDOW_AUTOSIZE);

	unique_ptr<UDPSender> sender = make_unique<UDPSender>(62731);

	FaceDetector faceDetector(camera, "shape_predictor_68_face_landmarks.dat");

	// CVUI
	
	bool uiBoolTransmitHead = true, uiBoolTransmitHands = true, uiBoolTransmitFacial = true;
	uiBoolTransmitHead = args["head"];
	uiBoolTransmitHands = args["apriltags"];
	uiBoolTransmitFacial = args["facial"];

	if (!(uiBoolTransmitHead || uiBoolTransmitHands || uiBoolTransmitFacial))
	{
		// if nothng is enabled, lets enable the basic thing
		uiBoolTransmitHead = true;
	}

	//bool uiBoolTransmitHeadTracker = false;
	int requestResolution = -1;
	Mat uiframe;
	{
		cvui::init(windowTitle);
	}
	// CVUI end

	// performance metrics
	double avgFrameTime = 0.0;
	int64 prev_frame_start = -1;
	const double tickFreq = cv::getTickFrequency();

	PerformanceMonitor pf_total("Total");
	pf_total.begin();

	PerformanceMonitor pf_camera("Camera");
	PerformanceMonitor pf_face("Face");
	PerformanceMonitor pf_apriltags("Apriltags");
	//

	cv::Mat frame;

	while (1) {

		// change resolution, if user clicked so
		if (-1 != requestResolution) {
			camera->requestNewResolution(requestResolution);
			requestResolution = -1;
		}

		{ // grab from camera
			pf_camera.begin();
			camera->capture();
			frame = camera->getFrame();
			pf_camera.end();
			if (frame.empty()) break;
		}

		// apriltags (hands, maybe chest)
		if(uiBoolTransmitHands)  
		{ 
			pf_apriltags.begin();
			aprilTagTriplets.precalc(frame);
			pf_apriltags.end();
		}

		// face detection
		bool isThereFace = false;
		Mat face_rvec, face_tvec;
		{
			int downscale = 2;
			cv::Mat frame_small;
			cv::resize(frame, frame_small, frame.size() / downscale);

			pf_face.begin();
			isThereFace = faceDetector.detect(frame, frame_small, downscale, face_rvec, face_tvec);
			pf_face.end();
		}

		sender->begin();

		/*if (isThereFace && uiBoolTransmitHeadTracker)
		{
			// this for HeadTracker plugin
			std::vector<double> data;
			data.push_back(face_tvec.at<double>(0));
			data.push_back(face_tvec.at<double>(1));
			data.push_back(face_tvec.at<double>(2));
			data.push_back(face_rvec.at<double>(0));
			data.push_back(face_rvec.at<double>(1));
			data.push_back(face_rvec.at<double>(2));

			sender->add_packet66(data);
		}*/

		if (isThereFace && uiBoolTransmitHead)
			sender->add_head(face_rvec, face_tvec);

		// facial
		if (uiBoolTransmitFacial && isThereFace)
		{
			array< pair<short, short>, 68 >* facialPose = faceDetector.getLatestPose();
			if (nullptr != facialPose)
				sender->add_68points(*facialPose);
		}
		
		//apriltags
		if(uiBoolTransmitHands)
		{ 
			Mat rvec, tvec;
			vector<float> left, right;

			if (aprilTagTriplets.findTriplet(11, 12, 13, rvec, tvec))
			{
				drawFrameAxes(frame, camera->getCameraK(), camera->getCameraD(), rvec, tvec, 3);

				sender->add_right_hand(rvec, tvec);
			}

			if (aprilTagTriplets.findTriplet(21, 22, 23, rvec, tvec))
			{
				double leftHandMatrixValues[9] = { 1.0,0,0  ,  0,1.0,0  ,  0,0,1.0 };
				Mat leftHandMatrix(3, 3 , CV_64FC1, leftHandMatrixValues);

				drawFrameAxes(frame, camera->getCameraK(), camera->getCameraD(), rvec, tvec, 3);

				sender->add_left_hand(rvec, tvec);
			}
		}

		sender->send();

		{
			pf_total.end();
			pf_total.begin();

			cv::putText(frame, pf_total.getStringFPS() , cv::Point(10, 15), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 255, 0), 1);

			cv::putText(frame, pf_camera.getStringPercents(), cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 255, 0), 1);
			cv::putText(frame, pf_face.getStringPercents(), cv::Point(10, 45), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 255, 0), 1);
			cv::putText(frame, pf_apriltags.getStringPercents(), cv::Point(10, 60), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 255, 0), 1);
		}


		if (0 == cv::getWindowProperty(windowTitle, cv::WND_PROP_VISIBLE))  break; // 'X' was clicked

		Mat resframe;
		{ // CVUI
			if (uiframe.cols != frame.cols)
				uiframe = Mat(120, frame.cols, frame.type(), cv::Scalar(49, 52, 49));
			
			cv::vconcat(uiframe, frame, resframe);

			{
				auto uiLine = [](int x) -> int { return x * 15 - 10; };
				cvui::text(resframe, 10, uiLine(1), "Detect and transmit following data:");
				cvui::checkbox(resframe, 10, uiLine(2), "Head location and rotation", &uiBoolTransmitHead);
				cvui::checkbox(resframe, 10, uiLine(3), "Hands location and rotation", &uiBoolTransmitHands);
				cvui::checkbox(resframe, 10, uiLine(4), "Facial metrics (emotions)", &uiBoolTransmitFacial);
				//cvui::checkbox(resframe, 10, uiLine(5), "The data for HeadTracker plugin (v0.5 or lateryt)", &uiBoolTransmitHeadTracker);
			}

			{
				auto uiLine = [](int x) -> int { return  (x==1)?5 : (x-2) * 30 + 20; };
				cvui::text(resframe, 310, uiLine(1), "Request resolution from camera:");
				if(cvui::button(resframe, 310, uiLine(2), "640 x 480")) requestResolution = 640;
				if(cvui::button(resframe, 310, uiLine(3), "1280 x 768") ) 
					requestResolution = 1280;
				if (cvui::button(resframe, 310, uiLine(4), "1920 x 1080")) requestResolution = 1920;
			}
			

			cvui::update();
		}


		cv::imshow(windowTitle, resframe );

		if ((char)cv::waitKey(1) >= 0) break;
	}

	return 0;
}

