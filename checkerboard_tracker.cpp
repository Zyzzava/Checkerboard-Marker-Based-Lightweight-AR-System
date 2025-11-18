#include "checkerboard_tracker.hpp"

namespace
{
    // Define the checkerboard pattern size and detection flags
    const cv::Size kPatternSize{8, 6};
    // Combination of flags for checkerboard detection
    constexpr int kFlags = cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE;
}

namespace ar
{
    bool detectCheckerboard(cv::Mat &frame, std::vector<cv::Point2f> &corners)
    {
        const bool found = cv::findChessboardCorners(frame, kPatternSize, corners, kFlags);
        if (found)
        {
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            cv::cornerSubPix(gray, corners, {11, 11}, {-1, -1},
                             cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::COUNT, 30, 0.1));
            cv::drawChessboardCorners(frame, kPatternSize, corners, found);
        }
        return found;
    }
}