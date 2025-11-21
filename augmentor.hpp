#include <opencv2/opencv.hpp>
#include <vector>

void initAugmentor(cv::Mat &cameraMatrix, cv::Mat &distCoeffs, cv::Size patternSize);
void augmentLoop(cv::VideoCapture &capture, bool &useNft, cv::Size patternSize, float squareSize);