#include "augmentor.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <opencv2/opencv.hpp>

namespace
{
    const std::filesystem::path kCalibrationJson{"data/calibration/calibration.json"};

    cv::Mat jsonToMat(const nlohmann::json &json)
    {
        if (!json.is_array() || json.empty())
        {
            return {};
        }

        cv::Mat mat(static_cast<int>(json.size()), static_cast<int>(json[0].size()), CV_64F);
        for (size_t r = 0; r < json.size(); ++r)
        {
            for (size_t c = 0; c < json[r].size(); ++c)
            {
                mat.at<double>(static_cast<int>(r), static_cast<int>(c)) = json[r][c].get<double>();
            }
        }
        return mat;
    }

    bool loadCalibrationData(const std::filesystem::path &path,
                             cv::Mat &cameraMatrix,
                             cv::Mat &distCoeffs,
                             std::vector<cv::Point3f> &objectPoints)
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        std::ifstream in(path);
        if (!in)
        {
            return false;
        }

        nlohmann::json data;
        in >> data;

        cameraMatrix = jsonToMat(data["camera_matrix"]);
        distCoeffs = jsonToMat(data["distortion_coefficients"]);

        objectPoints.clear();
        for (const auto &entry : data["object_points_template"])
        {
            if (entry.is_array() && entry.size() == 3)
            {
                objectPoints.emplace_back(entry[0].get<double>(),
                                          entry[1].get<double>(),
                                          entry[2].get<double>());
            }
        }

        return !cameraMatrix.empty() && !distCoeffs.empty() && !objectPoints.empty();
    }
}

void augmentLoop(cv::VideoCapture &capture)
{
    // initialize augmentor
    cv::Mat cameraMatrix, distCoeffs;
    std::vector<cv::Point3f> objectPoints;
    initAugmentor(cameraMatrix, distCoeffs, objectPoints);
    if (cameraMatrix.empty() || distCoeffs.empty() || objectPoints.empty())
    {
        std::cerr << "Failed to load calibration data." << std::endl;
        return;
    }
    // start video capture
    cv::VideoCapture cap(0);
    if (!cap.isOpened())
    {
        std::cerr << "Error: Could not open camera." << std::endl;
        return;
    }
    // start loop
    cv::Size patternSize(8, 6);
    float squareSize = 30.0f; // in mm
    cv::Mat frame;
    while (true)
    {
        // capture frame
        cap >> frame;
        if (frame.empty())
        {
            std::cerr << "Error: Could not read frame." << std::endl;
            break;
        }
        // estimate pose
        std::vector<cv::Point2f> corner_pts;
        bool success = cv::findChessboardCorners(frame, patternSize, corner_pts);
        if (success)
        {
            cv::Mat rvec, tvec;
            cv::solvePnP(objectPoints, corner_pts, cameraMatrix, distCoeffs, rvec, tvec);

            // render something idk:
            std::vector<cv::Point3f> axisPoints;
            axisPoints.push_back(cv::Point3f(0, 0, 0));               // Origin (for the circle)
            axisPoints.push_back(cv::Point3f(squareSize * 3, 0, 0));  // X-axis
            axisPoints.push_back(cv::Point3f(0, squareSize * 3, 0));  // Y-axis
            axisPoints.push_back(cv::Point3f(0, 0, -squareSize * 3)); // Z-axis

            std::vector<cv::Point2f> image_axes;
            cv::projectPoints(axisPoints, rvec, tvec, cameraMatrix, distCoeffs, image_axes);

            // Draw the projected axes on the image
            cv::line(frame, image_axes[0], image_axes[1], cv::Scalar(0, 0, 255), 3); // X-axis in Red
            cv::line(frame, image_axes[0], image_axes[2], cv::Scalar(0, 255, 0), 3); // Y-axis in Green
            cv::line(frame, image_axes[0], image_axes[3], cv::Scalar(255, 0, 0), 3); // Z-axis in Blue
        }
        cv::imshow("AR View", frame);
        if (cv::waitKey(1) == 27)
        { // ESC key
            break;
        }
    }
}

void initAugmentor(cv::Mat &cameraMatrix, cv::Mat &distCoeffs, std::vector<cv::Point3f> &objectPoints)
{
    if (!loadCalibrationData(kCalibrationJson, cameraMatrix, distCoeffs, objectPoints))
    {
        std::cerr << "Unable to read " << kCalibrationJson << std::endl;
    }
}