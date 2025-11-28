#include <vector>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>

struct FrameStats
{
    int frame_id;
    double timestamp;
    bool poseSuccess;
    cv::Mat rvec, tvec;
    double frameTimeMs;
};

struct SessionStats
{
    std::vector<FrameStats> frames;

    // Compute pose stability metrics
    nlohmann::json computePoseStability() const;
    // Compute detection robustness metrics
    nlohmann::json computeDetectionRobustness() const;
    // Compute computational performance metrics
    nlohmann::json computePerformance() const;

    // Export all metrics as JSON
    nlohmann::json toJson() const;
};