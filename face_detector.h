
class FaceDetector {

	FaceDetector(const FaceDetector&) = delete;
	FaceDetector& operator=(FaceDetector const&) = delete;

	//constants
	static const int FACE_POINTS = 12;
	static const int SOLUTIONS = 5; // 1 or 5 or 9

	static const double face_model[FACE_POINTS][3];
	static const int face_model_indeces[FACE_POINTS];
	static const int adjacents_and_center[SOLUTIONS][2];

	std::vector<cv::Point3d> cv_face_model;

	//setup
	//cv::Mat mat_camera_K, mat_camera_D;
	shared_ptr<Camera> camera;

	dlib::frontal_face_detector frontal_face_detector;
	dlib::shape_predictor pose_model;


	//inter-frame state

	dlib::rectangle prevFaceRect;
	bool prevFaceRectValid = false;
	cv::Mat prevRotation, prevTranslation;
	bool prevRotationAndTranslationValid = false;

	array< pair<short, short>, 68 > latest_pose;
	bool latest_pose_valid = false;

public:

	void reset();
	//FaceDetector(cv::Mat & camera_K, cv::Mat & camera_D, const char * path_to_shape_predictor);
	FaceDetector(shared_ptr<Camera> camera, const char* path_to_shape_predictor);

	bool detect(cv::Mat& frame, cv::Mat& frame_small, int downscale, cv::Mat& rvec, cv::Mat& tvec);

	array< pair<short, short>, 68 > * getLatestPose();

};
