#include <opencv2/opencv.hpp>

namespace ar
{
    void calibrateCamera(cv::VideoCapture &capture, int requiredSamples = 15,
                         const std::string &outputDir = "calibration_images", cv::Size patternSize = cv::Size(8, 6), float squareSize = 25.0f);

    void captureReferenceImage(cv::VideoCapture &capture,
                               const std::string &outputDir);
}