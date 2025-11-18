#pragma once

#include <opencv2/opencv.hpp>

namespace ar
{
    bool initializeCapture(cv::VideoCapture &capture, int deviceIndex = 0);
    void captureLoop(cv::VideoCapture &capture);
} // namespace ar