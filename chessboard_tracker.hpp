#include "tracker.hpp"

// Implements pose estimation using a chessboard pattern
class ChessboardTracker : public PoseTracker
{
    cv::Size patternSize;                  // Number of inner corners per chessboard row and column
    float squareSize;                      // Size of a square in the chessboard pattern (e.g., in millimeters)
    std::vector<cv::Point3f> objectPoints; // 3D points in the chessboard coordinate space

public:
    std::vector<cv::Point2f> lastCorners; // Last detected corners
    // Constructor
    ChessboardTracker(cv::Size size, float sqSize) : patternSize(size), squareSize(sqSize) {}
    // Initialize the tracker by preparing object points
    void init() override
    {
        // Prepare object points based on the chessboard pattern size and square size
        objectPoints.clear();
        // Center the chessboard at the origin
        float cx = (patternSize.width - 1) * squareSize / 2.0f;
        float cy = (patternSize.height - 1) * squareSize / 2.0f;
        // Generate 3D points
        for (int i = 0; i < patternSize.height; i++)
        {
            for (int j = 0; j < patternSize.width; j++)
            {
                // Center the points around the origin
                objectPoints.push_back(cv::Point3f(j * squareSize - cx, i * squareSize - cy, 0));
            }
        }
    }

    // Estimate pose from the given frame
    bool estimatePose(const cv::Mat &frame, const cv::Mat &camMat, const cv::Mat &dist, cv::Mat &rvec, cv::Mat &tvec) override
    {
        // Convert to grayscale
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Find chessboard corners
        std::vector<cv::Point2f> corners;
        bool found = cv::findChessboardCorners(gray, patternSize, corners, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

        if (found)
        {
            // Refine corners (Sub-pixel)
            cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                             cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 30, 0.1));
            lastCorners = corners; // Store the detected corners

            // Draw detected corners for debugging
            cv::Mat debugImg = frame.clone();
            cv::drawChessboardCorners(debugImg, patternSize, corners, found);
            cv::imshow("Chessboard Detection", debugImg);

            // Calculate Pose
            cv::solvePnP(objectPoints, corners, camMat, dist, rvec, tvec);
            return true;
        }
        else
        {
            lastCorners.clear(); // Clear if not found
        }
        return false;
    }
};