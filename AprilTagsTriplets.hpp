


class AprilTagsTriplets {

	cv::Ptr<cv::aruco::Dictionary> arDict;
	cv::Ptr<cv::aruco::DetectorParameters> detectorParams;
	
	shared_ptr<Camera> camera;

	std::vector< std::vector<cv::Point2f> > corners;
	std::vector<int> ids;

public :
	
	void init(shared_ptr<Camera> camera);

	void precalc(cv::Mat const& img);

	boolean findTriplet(int n1, int n2, int n3, cv::Mat& rvec, cv::Mat& tvec);

};