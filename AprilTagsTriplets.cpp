#include "stdafx.h"
#include "after_stdafx.h"

using namespace std;
using namespace cv;



void AprilTagsTriplets::init(shared_ptr<Camera> camera)
{
	this->camera = camera;

	arDict = aruco::getPredefinedDictionary(aruco::DICT_APRILTAG_25h9);

	detectorParams = aruco::DetectorParameters::create();
	//detectorParams->cornerRefinementMethod = aruco::CORNER_REFINE_APRILTAG;

}


void AprilTagsTriplets::precalc(Mat const& frame)
{
	aruco::detectMarkers(frame, arDict, corners, ids, detectorParams, cv::noArray(), camera->getCameraK(), camera->getCameraD());
}

boolean AprilTagsTriplets::findTriplet(int n1, int n2, int n3, Mat& rvec, Mat& tvec)
{
	if (std::any_of(ids.begin(), ids.end(), [n1, n2, n3](int id) { return id == n1 || id == n2 || id == n3; }))
	{
		double s = 5.45 / 2; // size of aruco square, in centimeters
		double b = 0.8 / sqrt(2); // distance between squares, straight line through edge

		vector<cv::Point3d>	objectPoints;
		vector<cv::Point2d> imagePoints;

		{ // n1
			auto pos = std::find(ids.begin(), ids.end(), n1);
			if (pos != ids.end()) {
				objectPoints.push_back(Point3d(s + b, s, s));
				objectPoints.push_back(Point3d(s + b, s, -s));
				objectPoints.push_back(Point3d(s + b, -s, -s));
				objectPoints.push_back(Point3d(s + b, -s, s));

				vector<Point2f>& c = corners[pos - ids.begin()];
				imagePoints.push_back(c[0]);
				imagePoints.push_back(c[1]);
				imagePoints.push_back(c[2]);
				imagePoints.push_back(c[3]);
			}
		}
		
		{ // n2
			auto pos = std::find(ids.begin(), ids.end(), n2);
			if (pos != ids.end()) {
				objectPoints.push_back(Point3d(s, s + b, s));
				objectPoints.push_back(Point3d(-s, s + b, s));
				objectPoints.push_back(Point3d(-s, s + b, -s));
				objectPoints.push_back(Point3d(s, s + b, -s));

				vector<Point2f>& c = corners[pos - ids.begin()];
				imagePoints.push_back(c[0]);
				imagePoints.push_back(c[1]);
				imagePoints.push_back(c[2]);
				imagePoints.push_back(c[3]);
			}
		}
		
		{ // n3
			auto pos = std::find(ids.begin(), ids.end(), n3);
			if (pos != ids.end()) {
				objectPoints.push_back(Point3d(s, s, s + b));
				objectPoints.push_back(Point3d(s,- s, s + b));
				objectPoints.push_back(Point3d(-s, -s, s + b));
				objectPoints.push_back(Point3d(-s, s, s + b));

				vector<Point2f>& c = corners[pos - ids.begin()];
				imagePoints.push_back(c[0]);
				imagePoints.push_back(c[1]);
				imagePoints.push_back(c[2]);
				imagePoints.push_back(c[3]);
			}
		}

		{
			Point3d shift(s + b, s + b, s + b);
			for (auto& q : objectPoints)  q -= shift;
		}

		cv::solvePnP(objectPoints, imagePoints, camera->getCameraK(), camera->getCameraD(), rvec, tvec);

		return true;
	}

	return false;
}