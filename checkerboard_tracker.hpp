#include <opencv2/opencv.hpp>

namespace ar
{
    bool detectCheckerboard(cv::Mat &frame, std::vector<cv::Point2f> &outCorners);
}