#include <fstream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "calibrator.hpp"
#include "augmentor.hpp"

using namespace ar;

int main()
{
    cv::VideoCapture capture;

    const std::filesystem::path calibrationJson{"data/calibration/calibration.json"};
    if (!std::filesystem::exists(calibrationJson))
    {
        calibrateCamera(capture, 15, "calibration");
    }

    augmentLoop(capture);

    return 0;
}