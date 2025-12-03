#include <opencv2/opencv.hpp>
#include <vector>

// Initialize augmentor by loading camera calibration data
void initAugmentor(cv::Mat &cameraMatrix, cv::Mat &distCoeffs, cv::Size patternSize);
// Main augmentation loop - captures video, estimates pose, and renders AR content
void augmentLoop(cv::VideoCapture &capture, bool &useNft, cv::Size patternSize, float squareSize, const std::string &experimentName, const std::string &testName);