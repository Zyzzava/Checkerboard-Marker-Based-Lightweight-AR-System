#include "capture_helpers.hpp"
#include "checkerboard_tracker.hpp"

#include <iostream>

namespace
{
    constexpr auto kWindowName = "Live Capture";

    bool shouldExit(int key)
    {
        return key == 27 || key == 'q' || key == 'Q';
    }
} // namespace

namespace ar
{
    bool initializeCapture(cv::VideoCapture &capture, int deviceIndex)
    {
        capture.open(deviceIndex);
        if (!capture.isOpened())
        {
            std::cerr << "Unable to open camera device " << deviceIndex << "\n";
            return false;
        }

        cv::namedWindow(kWindowName, cv::WINDOW_AUTOSIZE);
        std::cout << "Press ESC or 'q' to quit.\n";
        return true;
    }

    void captureLoop(cv::VideoCapture &capture)
    {
        std::vector<cv::Point2f> corners;
        while (true)
        {
            cv::Mat frame;
            if (!capture.read(frame))
            {
                std::cerr << "Camera read failed.\n";
                break;
            }

            if (ar::detectCheckerboard(frame, corners))
            {
                std::cout << "Checkerboard detected with " << corners.size() << " corners.\n";
            }

            cv::imshow(kWindowName, frame);
            if (shouldExit(cv::waitKey(3)))
            {
                break;
            }
        }
    }
} // namespace ar