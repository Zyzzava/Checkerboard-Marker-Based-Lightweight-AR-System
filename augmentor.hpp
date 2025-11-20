#include <opencv2/opencv.hpp>
#include <vector>

void initAugmentor(cv::Mat &cameraMatrix, cv::Mat &distCoeffs);
void augmentLoop(cv::VideoCapture &capture);