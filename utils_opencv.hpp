#pragma once


inline void makeRotatedMat(const cv::Mat src, cv::Mat& dst, double angle, cv::Mat& directRot)
{
    double angleRad = angle * 3.1415 / 180;
    float w = src.size().width, h = src.size().height;
    cv::Point2f srcPoints[4] = { {0,0},{w,0},{0,h},{w,h} };
    //cv::Point2f srcPoints[8] = { {w/3,0},{2*w/3,0},{w/3,h},{0,h/3},{0,2*h/3},{2*w/3,h},{w,h/3},{w,2*h/3}};
    cv::Point2f dstPoints[4];
    float minx = 1e5, miny = 1e5, maxx = -1e5, maxy = -1e5;
    for (int i = 0;i < 4;i++) {
        dstPoints[i].x = srcPoints[i].x * cos(angleRad) - srcPoints[i].y * sin(angleRad);
        dstPoints[i].y = srcPoints[i].x * sin(angleRad) + srcPoints[i].y * cos(angleRad);
        minx = std::min(minx, dstPoints[i].x);
        miny = std::min(miny, dstPoints[i].y);
    }
    for (int i = 0;i < 4;i++) {
        dstPoints[i].x -= minx;
        dstPoints[i].y -= miny;
        maxx = std::max(maxx, dstPoints[i].x);
        maxy = std::max(maxy, dstPoints[i].y);
    }

    directRot = cv::getAffineTransform(srcPoints, dstPoints);
    //cv::invertAffineTransform(directRot,inverseRot);

    cv::warpAffine(src, dst, directRot, cv::Size(maxx, maxy), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
}


inline string MatType2str(int type) {
    string r;

    uchar depth = type & CV_MAT_DEPTH_MASK;
    uchar chans = 1 + (type >> CV_CN_SHIFT);

    switch (depth) {
    case CV_8U:  r = "8U"; break;
    case CV_8S:  r = "8S"; break;
    case CV_16U: r = "16U"; break;
    case CV_16S: r = "16S"; break;
    case CV_32S: r = "32S"; break;
    case CV_32F: r = "32F"; break;
    case CV_64F: r = "64F"; break;
    default:     r = "User"; break;
    }

    r += "C";
    r += (chans + '0');

    return r;
}

// transforms ( =traslation+rotation ) a Point of two doubles with 2x3 transformation matrix
inline cv::Point2d transform_d(const cv::Point2d p, const cv::Mat rot)
{
    if (rot.type() != CV_64F)
    {
        cerr << "Runtime error FNFIDNFJEB" << endl;
        exit(13);
    }
    return cv::Point2d(
        p.x * rot.at<double>(0, 0) + p.y * rot.at<double>(0, 1) + rot.at<double>(0, 2)
        , p.x * rot.at<double>(1, 0) + p.y * rot.at<double>(1, 1) + rot.at<double>(1, 2)
    );
}

inline cv::Point2f transform_f(const cv::Point2f p, const cv::Mat rot)
{
    if (rot.type() != CV_32F)
    {
        cerr << "Runtime error FNFIDNFJEB" << endl;
        exit(13);
    }
    return cv::Point2f(
        p.x * rot.at<float>(0, 0) + p.y * rot.at<float>(0, 1) + rot.at<float>(0, 2)
        , p.x * rot.at<float>(1, 0) + p.y * rot.at<float>(1, 1) + rot.at<float>(1, 2)
    );
}

inline cv::RotatedRect transformRR(const cv::RotatedRect r, const cv::Mat rot)
{

    // here I use RotatedRect as an oriented rect, although RotatedRect is not defined as such.
    cv::Point2f points[4];
    r.points(points);
    return cv::RotatedRect(
        transform_d(points[0],rot),
        transform_d(points[1],rot),
        transform_d(points[2],rot)
        );

}

inline cv::RotatedRect scaleRR(const cv::RotatedRect rr, float scale)
{
    return cv::RotatedRect(rr.center * scale, rr.size * scale, rr.angle);
}

inline void drawRotatedRect(const cv::Mat dst, const cv::RotatedRect rr, const cv::Scalar color)
{
    cv::Point2f rrPoints[5];
    rr.points(rrPoints);
    rrPoints[4] = rrPoints[0];
    for (int i = 0;i < 4;i++)
        cv::line(dst, rrPoints[i + 0], rrPoints[i + 1], color);
}


inline void extractRotatedSubMat(const cv::Mat src, cv::Mat& dst, cv::RotatedRect region, cv::Mat& directRot)
{
    cv::Rect bounds = region.boundingRect();

    cv::Point2f regPoints[4];
    region.points(regPoints);

    cv::Point2f dstPoints[3] = { {0,region.size.height}, {0,0} , {region.size.width,0} };


    directRot = cv::getAffineTransform(regPoints, dstPoints);

    cv::warpAffine(src, dst, directRot, region.size, cv::INTER_LINEAR);
}

inline cv::Rect cvtRect(dlib::rectangle const& r) {
    return cv::Rect(r.left(), r.top(), r.width(), r.height());
}

inline cv::Point cvtPoint(dlib::point const & p) {
    
    return cv::Point( (int)p.x(), (int)p.y());
}

