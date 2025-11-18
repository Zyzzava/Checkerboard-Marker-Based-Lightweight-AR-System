#include <opencv2/opencv.hpp>
#include "capture_helpers.hpp"

using namespace ar;

int main()
{
    cv::VideoCapture capture;
    if (!initializeCapture(capture))
    {
        return 1;
    }

    captureLoop(capture);
    return 0;
}