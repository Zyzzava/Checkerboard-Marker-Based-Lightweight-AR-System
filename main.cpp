#include <opencv2/opencv.hpp>
#include "calibrator.hpp"
#include <fstream>

using namespace ar;

int main()
{
    cv::VideoCapture capture;

    std::ifstream intrinsics_file("intrinsics.xml");
    if (!intrinsics_file.good())
    {
        calibrateCamera(capture, 15, "calibration");
    }

    return 0;
}