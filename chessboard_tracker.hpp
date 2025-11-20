#include "tracker.hpp"

class ChessboardTracker : public PoseTracker
{
    cv::Size patternSize;
    float squareSize;
    std::vector<cv::Point3f> objectPoints;

public:
    ChessboardTracker(cv::Size size, float sqSize) : patternSize(size), squareSize(sqSize) {}

    void init() override
    {
        // Pre-calculate the 3D positions of the corners
        for (int i = 0; i < patternSize.height; i++)
        {
            for (int j = 0; j < patternSize.width; j++)
            {
                objectPoints.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
            }
        }
    }

    bool estimatePose(const cv::Mat &frame, const cv::Mat &camMat, const cv::Mat &dist, cv::Mat &rvec, cv::Mat &tvec) override
    {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCorners(gray, patternSize, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

        if (found)
        {
            // Refine corners (Sub-pixel)
            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                             cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));

            // Calculate Pose
            cv::solvePnP(objectPoints, corners, camMat, dist, rvec, tvec);
            return true;
        }
        return false;
    }
};