#include <vector>
#include <opencv2/opencv.hpp>
#include <nlohmann/json.hpp>

struct FrameStats
{
    int frame_id;       // Unique frame identifier
    double timestamp;   // Timestamp in seconds
    bool poseSuccess;   // Whether pose estimation was successful
    cv::Mat rvec, tvec; // Rotation and translation vectors
    double frameTimeMs; // Time taken to process the frame in milliseconds
};

struct SessionStats
{
    // Collection of frame statistics
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