#include <opencv2/opencv.hpp>

namespace ar
{
    void calibrateCamera(cv::VideoCapture &capture, int requiredSamples = 15,
                         const std::string &outputDir = "calibration_images");

    void captureReferenceImage(cv::VideoCapture &capture,
                                const std::string &outputDir);
}