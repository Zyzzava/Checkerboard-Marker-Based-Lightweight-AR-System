#include <fstream>
#include <filesystem>
#include <opencv2/opencv.hpp>
#include "calibrator.hpp"
#include "augmentor.hpp"

using namespace ar;

int main()
{
    bool useNft = false;         // Set to true to use NFT, false for chessboard
    cv::VideoCapture capture(0); // Open camera here

    if (!capture.isOpened())
    {
        std::cerr << "Could not open camera!" << std::endl;
        return -1;
    }

    cv::Size patternSize(8, 6); // Number of inner corners per a chessboard row and column
    float squareSize = 25.0f;   // Set your physical square size here
    int requiredSamples = 15;

    // Check if calibration data exists; if not, run calibration
    // Build calibration folder path
    std::string patternStr = std::to_string(patternSize.width) + "x" + std::to_string(patternSize.height);
    const std::filesystem::path calibrationDir = "data/calibration/" + patternStr;

    const std::filesystem::path calibrationJson = calibrationDir / "calibration.json";
    if (!std::filesystem::exists(calibrationJson))
    {
        calibrateCamera(capture, requiredSamples, "calibration", patternSize, squareSize);
    }

    if (useNft)
    {
        if (!std::filesystem::exists("data/reference/reference.png"))
        {
            std::cout << "No reference image found for NFT. Capturing one now." << std::endl;
            captureReferenceImage(capture, "data/reference/");
        }
    }

    augmentLoop(capture, useNft, patternSize, squareSize);

    return 0;
}