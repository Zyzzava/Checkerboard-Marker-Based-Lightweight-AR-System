#include "jsonHelper.hpp"
#include <fstream>
#include <nlohmann/json.hpp>

namespace ar
{
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