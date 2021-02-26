#include "stdafx.h"
#include "after_stdafx.h"

using namespace cv;


const double FaceDetector::face_model[FACE_POINTS][3] = {
	{-5.8, 1, 0},   // left brow left corner       
	{5.8, 1, 0 },  // right brow right corner     

	{-4.7, 0, 0},   // outer side of left eye
	{-1.6, 0, 0},   // inner side of left eye
	{1.6, 0, 0},  // inner side of right eye
	{4.7, 0, 0},  // outer side of right eye

	{0.0, -5, 4},  // nose tip

	{-2, -5, 1},   // nose left corner            
	{2, -5, 1},  // nose right corner

	{0, -7, 0.5 },  // mouth center
	//{0, -12, 0},   // chin corner                  
	//{-4.5,-10,-1}, // chin left
	//{4.5,-10,-1}, // chin right

	{-7,-1,-7}, // left point near ear
	{7,-1,-7} // right point near ear



};

const int FaceDetector::face_model_indeces[FACE_POINTS] = { 17,26, 36,39,42,45, 30, 31,35, 51 /*,5,8,11*/ ,1,15 };

/*
const int FaceDetector::adjacents_and_center[SOLUTIONS][2] = { // 9 solutions
	{-1,-1},{-1,0},{-1,1},
	{0,-1},{0,0},{0,1},
	{1,-1},{1,0},{1,1}
};*/
const int FaceDetector::adjacents_and_center[SOLUTIONS][2] = { // 5 solutions
	{-1,-1},     {-1,1},
			{0,0},
	{1,-1},      {1,1},
};
/*const int FaceDetector::adjacents_and_center[SOLUTIONS][2] = { // 1 solution
			{0,0}
};*/




FaceDetector::FaceDetector(shared_ptr<Camera> camera,const char * path_to_shape_predictor)
{
	//this->mat_camera_K = mat_camera_K.clone();
	//this->mat_camera_D = mat_camera_D.clone();
	this->camera = camera;

	for (double const * xyz : face_model)   cv_face_model.push_back(Point3d( xyz[0], xyz[1], xyz[2]));

	frontal_face_detector = dlib::get_frontal_face_detector();
	dlib::deserialize(path_to_shape_predictor) >> pose_model;

	this->reset();
}

void FaceDetector::reset() {
	prevFaceRectValid = false;
	prevRotationAndTranslationValid = false;

}

bool FaceDetector::detect(cv::Mat & frame, cv::Mat & frame_small,int downscale, cv::Mat & face_rvec, cv::Mat& face_tvec)
{
	latest_pose_valid = false;

	vector<dlib::rectangle> faces;
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
			biggest_face.tl_corner() * downscale + faces_shift * downscale,
			biggest_face.br_corner() * downscale + faces_shift * downscale
		);

		vector<dlib::full_object_detection> poses;

		{
			for (int e = 0;e < SOLUTIONS;e++)
			{
				int dx = adjacents_and_center[e][0];
				int dy = adjacents_and_center[e][1];

				dlib::rectangle face_shifted(face.left() + dx, face.top() + dy, face.right() + dx, face.bottom() + dy);

				poses.push_back(pose_model(fullimg, face_shifted));
			}
		}

		array< pair<short, short>, 68 > avg_pose;
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
		latest_pose = avg_pose;
		latest_pose_valid = true;

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

		auto drawAnnotatedAvgPose = [](Mat& frame, const array< pair<short, short>, 68>& pose)
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

		auto drawPoseWithCircles = [](Mat& frame, const dlib::full_object_detection& pose, const int* model, int modelsize, int radius, const Scalar& color) {
			for (int i = 0;i < modelsize;i++)
			{
				Point2d p = (Point2d)cvtPoint(pose.part(model[i]));
				cv::circle(frame, p, radius, color, 1);
			}
		};

		for (int e = 0;e < SOLUTIONS;e++)
			drawPoseWithCircles(frame, poses[e], face_model_indeces, FACE_POINTS, 5, cv::Scalar(255, 255, 255));

		// solving for full frontal face
		vector<Mat> rotation_vec(SOLUTIONS);
		vector<Mat> translation_vec(SOLUTIONS);

		{
			for (int e = 0;e < SOLUTIONS;e++)
			{
				vector<Point2d> points;

				for (int i = 0;i < FACE_POINTS;i++)
					points.push_back((Point2d)cvtPoint(poses[e].part(face_model_indeces[i])));

				if (prevRotationAndTranslationValid) {
					rotation_vec[e] = prevRotation.clone();
					translation_vec[e] = prevTranslation.clone();
					solvePnP(cv_face_model, points, camera->getCameraK(), camera->getCameraD(), rotation_vec[e], translation_vec[e], true);
				}
				else {

					solvePnP(cv_face_model, points, camera->getCameraK(), camera->getCameraD(), rotation_vec[e], translation_vec[e], false);
				}
			}
		}

		auto normalize_t_r = [](Mat& t, Mat& r) {
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

		Mat res_translation;
		if (SOLUTIONS == 1)
			res_translation = translation_vec[0];
		else
			res_translation = selectTranslationVector(translation_vec);

		auto selectRotationVector = [](const vector<Mat>& v) -> Mat {

			vector<pair<double, int>> dists(v.size());
			for (int i = 0;i < v.size();i++)
				dists[i] = pair<double, int>(0.0, i);

			for (int i = 0;i < v.size() - 1;i++)
				for (int j = i + 1;j < v.size();j++)
				{
					double l = quaternion::angleDistanceMat(v[i], v[j]);
					dists[i].first += l;
					dists[j].first += l;
				}

			sort(dists.begin(), dists.end());

			quaternion q0(v[dists[0].second]);
			quaternion q1(v[dists[1].second]);
			quaternion q2(v[dists[2].second]);
			quaternion q3(v[dists[3].second]);
			quaternion q02 = q0.slerp(q2, 0.5);
			quaternion q13 = q1.slerp(q3, 0.5);
			quaternion q_fin = q02.slerp(q13, 0.5);

			return q_fin.toMat();
		};

		Mat res_rotation;
		if (SOLUTIONS == 1)
			res_rotation = rotation_vec[0];
		else
			res_rotation = selectRotationVector(rotation_vec);
		

		//output_t_r(res_translation, res_rotation);

		{
			drawFrameAxes(frame, camera->getCameraK(), camera->getCameraD(), res_rotation, res_translation, 10);
		}


		{ // returning result
			face_rvec = res_rotation.clone();
			face_tvec = res_translation.clone();
		}

		prevFaceRect = face;
		prevFaceRectValid = true;

		prevRotation = res_rotation;
		prevTranslation = res_translation;
		prevRotationAndTranslationValid = true;

		return true;  // face detected
	}

	return false; // no face detected

}


array< pair<short, short>, 68 > * FaceDetector::getLatestPose()
{
	if (latest_pose_valid) return &latest_pose;
	else return nullptr;
}