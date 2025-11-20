#ifndef JSON_HELPERS_HPP
#define JSON_HELPERS_HPP

#include <opencv2/opencv.hpp>
#include <vector>
#include <filesystem>

namespace ar
{
    bool loadCalibrationData(const std::filesystem::path &path,
                             cv::Mat &cameraMatrix,
                             cv::Mat &distCoeffs);
} // namespace ar

#endif // JSON_HELPERS_HPP