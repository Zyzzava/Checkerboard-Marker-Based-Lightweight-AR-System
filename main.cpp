#include <fstream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "calibrator.hpp"
#include "augmentor.hpp"

using namespace ar;

int main()
{
    bool useNft = false;

    cv::VideoCapture capture;

    const std::filesystem::path calibrationJson{"data/calibration/calibration.json"};
    if (!std::filesystem::exists(calibrationJson))
    {
        calibrateCamera(capture, 15, "calibration");
    }

    if (useNft)
    {
        // if no reference picture
        if (!std::filesystem::exists("data/reference/reference.png"))
        {
            std::cout << "No reference image found for NFT. Capturing one now." << std::endl;
            // capture and save reference picture
            captureReferenceImage(capture, "data/reference/");
        }
    }

    augmentLoop(capture, useNft);

    return 0;
}