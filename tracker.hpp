#pragma once
#include <opencv2/opencv.hpp>

class PoseTracker
{
public:
    virtual ~PoseTracker() = default;

    // Initializes the tracker (Load reference image or setup params)
    virtual void init() = 0;

    // Returns true if pose was found.
    // outputs: rvec and tvec (rotation and translation)
    virtual bool estimatePose(const cv::Mat &frame,
                              const cv::Mat &cameraMatrix,
                              const cv::Mat &distCoeffs,
                              cv::Mat &rvec,
                              cv::Mat &tvec) = 0;
};