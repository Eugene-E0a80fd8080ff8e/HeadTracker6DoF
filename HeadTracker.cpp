
#include "stdafx.h"

using namespace std;
using namespace cv;

#include "basic_quaternions.hpp"
#include "utils_opencv.hpp"
#include "udpsender.hpp"


double camera_K[9] = { -1, 0.0, -1, 0.0, -1, -1, 0.0, 0.0, 1.0 };
double camera_D[5] = { 0, 0, 0, 0, 0 };


const int FACE_POINTS = 15;
const int SOLUTIONS = 9;

double face_model[FACE_POINTS][3] = {
	{5.8, 1, 0},   // left brow left corner       
	{-5.8, 1, 0 },  // right brow right corner     

	{4.7, 0, 0},   // outer side of left eye
	{1.6, 0, 0},   // inner side of left eye
	{-1.6, 0, 0},  // inner side of right eye
	{-4.7, 0, 0},  // outer side of right eye

	{0.0, -5, 4},  // nose tip
	
	
	{2, -5, 1},   // nose left corner            
	{-2, -5, 1},  // nose right corner

	{0, -7, 0.5 },  // mouth center
	{0, -12, 0},   // chin corner                  
	{4.5,-10,-1}, // chin left
	{-4.5,-10,-1}, // chin right

	{7,-1,-7}, // left point near ear
	{-7,-1,-7} // right point near ear
	
};

int face_model_indeces[FACE_POINTS] = { 17,26, 36,39,42,45, 30, 31,35, 51,5,8,11 ,1,15};

int adjacents_and_center[SOLUTIONS][2] = {
	{-1,-1},{-1,0},{-1,1},
	{0,-1},{0,0},{0,1},
	{1,-1},{1,0},{1,1}
	/*{-1,-1},     {-1,1},
	        {0,0},
	{1,-1},      {1,1},*/
};

const char* const  windowTitle = "Web Cam Head Tracking";

int main(int argc, char *argv[])
{
	int selectedCamera = 0;
	int CAM_WIDTH = 640;
	int CAM_HEIGHT = 480;

	if (argc == 2)
	{
		sscanf(argv[1], "%d", &selectedCamera);
	}
	if (argc == 3)
	{
		int resolution =1;
		sscanf(argv[2], "%d", &resolution);
		if (resolution == 1) {
			CAM_WIDTH = 640;
			CAM_HEIGHT = 480;
		}
		else if(resolution == 2) {
			CAM_WIDTH = 800;
			CAM_HEIGHT = 600;
		}
		else if(resolution == 3) {
			CAM_WIDTH = 1024;
			CAM_HEIGHT = 768;
		}
		else if (resolution == 4) {
			CAM_WIDTH = 1280;
			CAM_HEIGHT = 800;
		}
		else if (resolution == 5) {
			CAM_WIDTH = 1920;
			CAM_HEIGHT = 1080;
		}
	}

	{
		camera_K[0] = CAM_WIDTH;
		camera_K[2] = CAM_WIDTH/2;
		camera_K[4] = CAM_WIDTH;
		camera_K[5] = CAM_HEIGHT/2;
	}

	std::cout << "Using camera #"<< selectedCamera << endl;


	cv::namedWindow(windowTitle, cv::WINDOW_AUTOSIZE);
	cv::VideoCapture cap;
	cap.open(selectedCamera);
	if (!cap.isOpened()) {
		std::cerr << "Error: Can not access the camera" << endl;
		exit(13);
	}

	//cap.set(cv::CAP_PROP_FOURCC, 'MJPG');
	//cap.set(cv::CAP_PROP_FOURCC, 'X264');
	cap.set(CAP_PROP_FRAME_WIDTH, CAM_WIDTH);
	cap.set(CAP_PROP_FRAME_HEIGHT, CAM_HEIGHT);
	//cap.set(cv::CAP_PROP_FPS,30);
	//cap.set(cv::CAP_PROP_FOURCC, MKFOURCC('M', 'J', 'P', 'G'));
	cap.set(CAP_PROP_BUFFERSIZE, 1);
	//cap.set(CAP_PROP_BUFFERSIZE, 0);

	Mat mat_camera_K = Mat(3, 3, CV_64FC1, camera_K);
	Mat mat_camera_D = Mat(5, 1, CV_64FC1, camera_D);

	vector<Point3d> cv_face_model;
	for (double* xyz : face_model)   cv_face_model.push_back(Point3d(xyz[0], xyz[1], xyz[2]));
	
	//std::unique_ptr<PipeSender> sender = move(factory_PipeSender());
	//sender->openPipe();
	unique_ptr<UDPSender> sender = make_unique<UDPSender>(62731);

	dlib::frontal_face_detector frontal_face_detector = dlib::get_frontal_face_detector();
	dlib::shape_predictor pose_model;
	dlib::deserialize("shape_predictor_68_face_landmarks.dat") >> pose_model;

	dlib::rectangle prevFaceRect;
	bool prevFaceRectValid = false;

	double avgFrameTime = 0.0;
	int64 prev_frame_start = -1;
	const double tickFreq = cv::getTickFrequency();

	Mat prevRotation, prevTranslation;
	bool prevRotationAndTranslationValid = false;


	while (1) {

		cv::Mat frame;
		cap >> frame;

		if (frame.empty()) break;
		int64 frame_start = cv::getTickCount();

		//cv::Mat frame_rgb;
		//cv::cvtColor(frame, frame_rgb, cv::COLOR_BGR2RGB);

		int downscale = 2;
		cv::Mat frame_small;
		cv::resize(frame, frame_small, frame.size() / downscale);

		//face_locations = frontal_face_detector()

		std::vector<dlib::rectangle> faces;
		dlib::point faces_shift;

		if (prevFaceRectValid)
		{
			//partial scan for a face

			dlib::rectangle frame_small_rectangle(frame_small.cols, frame_small.rows);

			int left = (prevFaceRect.left() / downscale - 15) & ~0xf;
			int top = (prevFaceRect.top() / downscale - 15) & ~0xf;
			int right = (prevFaceRect.right() / downscale + 31) & ~0xf;
			int bottom = (prevFaceRect.bottom() / downscale + 31) & ~0xf;
			dlib::rectangle face(left, top, right, bottom);

			dlib::rectangle search_in = frame_small_rectangle.intersect(face);
			faces_shift = search_in.tl_corner();
			dlib::cv_image<dlib::bgr_pixel> cimg(frame_small(cvtRect(search_in)));
			faces = frontal_face_detector(cimg);
		}

		if (faces.size() == 0)
		{
			// full frame scan
			dlib::cv_image<dlib::bgr_pixel> cimg(frame_small);
			faces = frontal_face_detector(cimg);
			faces_shift = dlib::point(0, 0);
		}


		if (faces.size() == 0)
		{
			prevFaceRectValid = false;
			prevRotationAndTranslationValid = false;
		}
		else {

			dlib::rectangle biggest_face = *std::max_element(faces.begin(), faces.end(),
				[](dlib::rectangle const& a, dlib::rectangle const& b) {
					return a.height() < b.height();
				}
			);

			dlib::cv_image<dlib::bgr_pixel> fullimg(frame);

			dlib::rectangle face(
				biggest_face.tl_corner()* downscale + faces_shift * downscale,
				biggest_face.br_corner()* downscale + faces_shift * downscale
			);

			vector<dlib::full_object_detection> poses;

			{
				for (int e = 0;e < SOLUTIONS;e++)
				{
					int dx = adjacents_and_center[e][0];
					int dy = adjacents_and_center[e][1];

					dlib::rectangle face_shifted(face.left() + dx, face.top() + dy, face.right() + dx, face.bottom() + dy);

					poses.push_back( pose_model(fullimg, face_shifted) );
				}
			}

			array< pair<short, short> , 68 > avg_pose;
			{
				for (int i = 0;i < (int)poses[0].num_parts();i++)
				{
					int x = 0, y = 0;
					for (auto pose : poses) {
						dlib::point p = pose.part(i);
						x += p.x();
						y += p.y();
					}
					x /= SOLUTIONS;
					y /= SOLUTIONS;
					avg_pose[i] = make_pair<short, short>((short)x, (short)y);
				}
			}

			auto drawAnnotatedPose = [](Mat& frame, const dlib::full_object_detection& pose)
			{
				for (int i = 0;i < (int)pose.num_parts();i++)
				{
					dlib::point p = pose.part(i);
					cv::circle(frame, cvtPoint(p), 4, cv::Scalar(128, 128, 128), -1);
					ostringstream str;
					str << i;
					cv::putText(frame, str.str(), cvtPoint(p) + cv::Point(-4, 2), cv::FONT_HERSHEY_PLAIN, 0.4, cv::Scalar(255, 255, 0), 1);
				}
			};
			//drawAnnotatedPose(frame, poses[0]);

			auto drawAnnotatedAvgPose = [](Mat& frame, const array< pair<short,short> , 68> & pose)
			{
				for (int i = 0;i < pose.size();i++)
				{
					Point p(pose[i].first, pose[i].second);

					cv::circle(frame, p, 3, cv::Scalar(128, 128, 128), -1);

					ostringstream str;
					str << i;
					cv::putText(frame, str.str(), p + cv::Point(-4, 2), cv::FONT_HERSHEY_PLAIN, 0.6, cv::Scalar(255, 255, 0), 1);
				}
			};
			drawAnnotatedAvgPose(frame, avg_pose);

			auto drawPoseWithCircles = [](Mat& frame, const dlib::full_object_detection& pose,int * model, int modelsize, int radius, const Scalar& color) {
				for (int i = 0;i < modelsize;i++)
				{
					Point2d p = (Point2d)cvtPoint(pose.part(model[i]));
					cv::circle(frame, p, radius, color, 1);
				}
			};

			for (int e = 0;e < SOLUTIONS;e++)
				drawPoseWithCircles(frame, poses[e], face_model_indeces, FACE_POINTS, 5, cv::Scalar(255, 255, 255));

			// solving for full frontal face
			vector<cv::Mat> rotation_vec(SOLUTIONS);
			vector<cv::Mat> translation_vec(SOLUTIONS);

			{
				for (int e = 0;e < SOLUTIONS;e++)
				{
					vector<Point2d> points;

					for (int i = 0;i < FACE_POINTS;i++)
						points.push_back((Point2d)cvtPoint(poses[e].part(face_model_indeces[i])));

					if (prevRotationAndTranslationValid) {
						rotation_vec[e] = prevRotation.clone();
						translation_vec[e] = prevTranslation.clone();
						solvePnP(cv_face_model, points, mat_camera_K, mat_camera_D, rotation_vec[e], translation_vec[e],true );
					}
					else {

						solvePnP(cv_face_model, points, mat_camera_K, mat_camera_D, rotation_vec[e], translation_vec[e], false);
					}
				}
			}

			auto normalize_t_r = [](Mat & t, Mat & r) {
				if (t.at<double>(2) < 0.0)
				{
					// flipping axii

					t = -t;

					Mat rm;
					Rodrigues(r, rm);
					Mat rrm = rm.inv();
					Mat flipM(3, 3, CV_64FC1, Scalar(0.0));
					flipM.at<double>(0, 0) = -1;
					flipM.at<double>(1, 1) = -1;
					flipM.at<double>(2, 2) = 1;

					Rodrigues(rrm * flipM * rm, r);
				}
			};

			for (int e = 0;e < SOLUTIONS;e++)
			{
				normalize_t_r(translation_vec[e], rotation_vec[e]);
			}
			

			auto output_t_r = [](const Mat& t, const Mat& r) {
				double tl = norm(t);
				printf("  %.1lf\t  %.1lf\t  %.1lf\t  %.1lf\n", t.at<double>(0), t.at<double>(1), t.at<double>(2), tl);

				double l = norm(r);
				printf("%.1lf\t%.1lf\t%.1lf\t%.1lf\n", r.at<double>(0), r.at<double>(1), r.at<double>(2), l);
			};

			/*
			for (int e = 0;e <= FACE_POINTS;e++)
				output_t_r(translation_vec[e],rotation_vec[e])
			*/
		

			auto selectTranslationVector = [](const vector<Mat>& v) -> Mat {

				vector<pair<double, int>> dists(v.size());
				for (int i = 0;i < v.size();i++)
					dists[i] = pair<double, int>(0.0, i);

				Mat res(3, 1, CV_64FC1, Scalar(0.0));
				for (int i = 0;i < v.size() - 1;i++)
					for (int j = i + 1;j < v.size();j++)
					{
						double l = norm(v[i] - v[j]);
						dists[i].first += l;
						dists[j].first += l;
					}

				sort(dists.begin(), dists.end());

				for (int i = 0;i < 5;i++)       // 5 hardcoded
					res += v[dists[i].second];
				return res / 5;
			};

			Mat res_translation = selectTranslationVector(translation_vec);

			auto selectRotationVector = [](const vector<Mat>& v) -> Mat {
				//using namespace irr::core;

				auto mat2quaternion = [](const Mat& m) -> quaternion {
					double l = norm(m);
					Mat mn = m / l;
					vector3 v(mn.at<double>(0), mn.at<double>(1), mn.at<double>(2));
					return quaternion::fromAngleAxis(l, v);
				};

				auto quaternion2mat = [](const quaternion& q) -> Mat {
					double angle;
					vector3 v;
					q.toAngleAxis(angle, v);
					angle += 2 * CV_PI;
					vector<double> d{ v.X * angle,v.Y * angle, v.Z * angle };
					return Mat(d, true);
				};

				auto angleDistanceMat = [mat2quaternion](const Mat& a, const Mat& b) {	// distance for rotation vectors
					quaternion qa = mat2quaternion(a);
					quaternion qb = mat2quaternion(b);
					return qa.angleDistance(qb);
				};


				vector<pair<double, int>> dists(v.size());
				for (int i = 0;i < v.size();i++)
					dists[i] = pair<double, int>(0.0, i);

				for (int i = 0;i < v.size() - 1;i++)
					for (int j = i + 1;j < v.size();j++)
					{
						double l = angleDistanceMat(v[i], v[j]);
						dists[i].first += l;
						dists[j].first += l;
					}

				sort(dists.begin(), dists.end());

				quaternion q0 = mat2quaternion(v[dists[0].second]);
				quaternion q1 = mat2quaternion(v[dists[1].second]);
				quaternion q2 = mat2quaternion(v[dists[2].second]);
				quaternion q3 = mat2quaternion(v[dists[3].second]);
				quaternion q02 = q0.slerp(q2, 0.5);
				quaternion q13 = q1.slerp(q3, 0.5);
				quaternion q_fin = q02.slerp(q13, 0.5);

				return quaternion2mat(q_fin);
			};

			Mat res_rotation = selectRotationVector(rotation_vec);

			//output_t_r(res_translation, res_rotation);

			{
				drawFrameAxes(frame, mat_camera_K, mat_camera_D, res_rotation, res_translation, 10);
			}

			/*
			{
				std::vector<std::pair<int,int> > data;
				for (int i = 0;i < pose.num_parts();i++)
				{
					dlib::point p = pose.part(i);
					data.push_back(std::make_pair(p.x(),p.y()));
				}
				sender.send(frame.cols, frame.rows, data);
			}
			*/
			{
				std::vector<double> data;
				data.push_back(res_translation.at<double>(0));
				data.push_back(res_translation.at<double>(1));
				data.push_back(res_translation.at<double>(2));
				data.push_back(res_rotation.at<double>(0));
				data.push_back(res_rotation.at<double>(1));
				data.push_back(res_rotation.at<double>(2));

				//sender->send_rt(data);
				sender->send_rt_and_68points(data,avg_pose);

			}

			prevFaceRect = face;
			prevFaceRectValid = true;

			prevRotation = res_rotation;
			prevTranslation = res_translation;
			prevRotationAndTranslationValid = true;
		}

		
		{ // benchmarking
			int64 frame_end = cv::getTickCount();
			ostringstream strTimespan;
			strTimespan << (frame_end - frame_start) / tickFreq * 1000 << " ms";
			cv::putText(frame, strTimespan.str(), cv::Point(10, 10), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 255, 0), 1);
		}
		

		{ // benchmarking
			if (prev_frame_start != -1)
			{
				double t = exp(-(frame_start - prev_frame_start) / tickFreq / 1.0); // tau = 1 second

				if (0 == avgFrameTime)
					avgFrameTime = (frame_start - prev_frame_start) / tickFreq;

				avgFrameTime = avgFrameTime * (t)+(frame_start - prev_frame_start) / tickFreq * (1 - t);

				ostringstream strFPS;
				strFPS << "avgFrameTime : " << (avgFrameTime * 1000) << " ms  ; FPS : " << 1 / avgFrameTime;
				cv::putText(frame, strFPS.str(), cv::Point(10, 30), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(0, 255, 0), 1);
			}
			prev_frame_start = frame_start;
		}

		
		if (0 == cv::getWindowProperty(windowTitle, cv::WND_PROP_VISIBLE))  break; // 'X' was clicked

		cv::imshow(windowTitle, frame);

		if ((char)cv::waitKey(1) >= 0) break;
	}

	return 0;
}

