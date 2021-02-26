





class vector3
{
public:
	double X, Y, Z;

	vector3() : X(0), Y(0), Z(0) {}
	vector3(double x, double y, double z) : X(x), Y(y), Z(z) {}

};


class quaternion
{
public:
	double X, Y, Z, W;

	quaternion() : X(0), Y(0), Z(0), W(1) {}
	quaternion(double x, double y, double z, double w) : X(x), Y(y), Z(z), W(w) {}


	static quaternion fromAngleAxis(double angle, vector3 axis)
	{
		double angle05 = 0.5 * angle;
		double q = sin(angle05);

		quaternion res;
		res.X = q * axis.X;
		res.Y = q * axis.Y;
		res.Z = q * axis.Z;
		res.W = cos(angle05);
		return res.normalize();
	}

	void toAngleAxis(double& angle, vector3& axis) const
	{
		double l = sqrt(X * X + Y * Y + Z * Z);

		if ( abs(l) < 1e-8  || W > 1 || W < -1)
		{
			angle = 0.0f;
			axis = vector3(0, 0, 1);
		}
		else
		{
			angle = 2 * acos(W);
			axis = vector3(X/l, Y/l, Z/l);
		}
	}

	quaternion inverse() const
	{
		return quaternion(-X,-Y,-Z,W);
	}

	double dot(const quaternion q2) const
	{
		return (X * q2.X) + (Y * q2.Y) + (Z * q2.Z) + (W * q2.W);
	}

	quaternion operator*(double scale) const
	{
		return quaternion(scale * X, scale * Y, scale * Z, scale * W);
	}

	quaternion operator*(const quaternion b) const
	{
		quaternion t;
		t.W = (b.W * W) - (b.X * X) - (b.Y * Y) - (b.Z * Z);
		t.X = (b.W * X) + (b.X * W) + (b.Y * Z) - (b.Z * Y);
		t.Y = (b.W * Y) + (b.Y * W) + (b.Z * X) - (b.X * Z);
		t.Z = (b.W * Z) + (b.Z * W) + (b.X * Y) - (b.Y * X);
		return t;
	}

	quaternion operator+(const quaternion& b) const
	{
		return quaternion(X + b.X, Y + b.Y, Z + b.Z, W + b.W);
	}

	quaternion slerp(const quaternion q2, double p) const
	{
		quaternion t = *this;
		double dp = t.dot(q2);

		if (dp < 0)
		{
			dp = -dp;
			t = quaternion(-X, -Y, -Z, -W);
		}

		double angle;
		if (dp > 1) angle = 0;
		else angle = acos(dp);

		double p1 = sin(angle * (1 - p)) / sin(angle);
		double p2 = sin(angle * p) / sin(angle);
		return (t * p1) + (q2 * p2);
	}

	double angleDistance(const quaternion b) const {
		double angle;
		vector3 v;
		(b.inverse() * *this).toAngleAxis(angle, v);
		return angle;
	};

	quaternion normalize() const
	{
		double l = X * X + Y * Y + Z * Z + W * W;

		return *this * (1.0/sqrt(l));
	}



	// OpenCV
	static quaternion fromMat_rvec(const cv::Mat & m) {
		double l = norm(m);
		cv::Mat mn = m / l;
		vector3 v(mn.at<double>(0), mn.at<double>(1), mn.at<double>(2));
		return quaternion::fromAngleAxis(l, v);
	}

	quaternion(const cv::Mat& m) {
		*this = fromMat_rvec(m);
	}

	cv::Mat toMat(){
		double angle;
		vector3 v;
		this->toAngleAxis(angle, v);
		angle += 2 * CV_PI;
		vector<double> d{ v.X * angle,v.Y * angle, v.Z * angle };
		return cv::Mat(d, true);
	}

	static double angleDistanceMat(const cv::Mat& a, const cv::Mat& b) {	// distance for rotation vectors
		quaternion qa = fromMat_rvec(a);
		quaternion qb = fromMat_rvec(b);
		return qa.angleDistance(qb);
	}


};