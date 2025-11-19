#include <opencv2/opencv.hpp>
#include <vector>

void initAugmentor(cv::Mat &cameraMatrix, cv::Mat &distCoeffs, std::vector<cv::Point3f> &objectPoints);
void augmentLoop(cv::VideoCapture &capture);