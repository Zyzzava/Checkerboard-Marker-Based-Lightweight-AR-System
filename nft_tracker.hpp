#include "tracker.hpp"
#include <opencv2/features2d.hpp>
#include <iostream>

class NFTTracker : public PoseTracker
{
    std::string imagePath;
    cv::Mat refImage;
    cv::Mat refDescriptors;
    std::vector<cv::KeyPoint> refKeypoints;
    std::vector<cv::Point3f> refObjectPoints; // The "3D" representation of the image pixels

    cv::Ptr<cv::Feature2D> detector;
    cv::Ptr<cv::DescriptorMatcher> matcher;

    // Scale factor to convert pixels to "World Units"
    // e.g., if image is 500px wide, and we want it to appear 50 units wide, scale = 0.1
    float scaleFactor = 0.1f;

public:
    NFTTracker(std::string path) : imagePath(path) {}

    void init() override
    {
        refImage = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
        if (refImage.empty())
        {
            std::cerr << "Could not load reference image!" << std::endl;
            return;
        }

        std::cout << "Reference image size: " << refImage.cols << "x" << refImage.rows << std::endl;
        std::cout << "World size with scale " << scaleFactor << ": "
                  << refImage.cols * scaleFactor << " x " << refImage.rows * scaleFactor << std::endl;

        // 1. Setup ORB Detector
        detector = cv::ORB::create(2000); // Track 2000 features
        matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");

        // 2. Analyze the Reference Image
        detector->detectAndCompute(refImage, cv::noArray(), refKeypoints, refDescriptors);

        // 3. Create 3D Object Points from 2D Keypoints
        // We assume the image is flat (Z=0).
        float centerX = refImage.cols * 0.5f;
        float centerY = refImage.rows * 0.5f;

        for (const auto &kp : refKeypoints)
        {
            // Subtract center to make origin at image center
            float x = (kp.pt.x - centerX) * scaleFactor;
            float y = (kp.pt.y - centerY) * scaleFactor;
            refObjectPoints.push_back(cv::Point3f(x, y, 0.0f));
        }
    }

    bool estimatePose(const cv::Mat &frame, const cv::Mat &camMat, const cv::Mat &dist, cv::Mat &rvec, cv::Mat &tvec) override
    {
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // 1. Detect features in current frame
        std::vector<cv::KeyPoint> currKeypoints;
        cv::Mat currDescriptors;
        detector->detectAndCompute(gray, cv::noArray(), currKeypoints, currDescriptors);

        if (currDescriptors.empty())
            return false;

        // 2. Match against reference
        std::vector<cv::DMatch> matches;
        matcher->match(refDescriptors, currDescriptors, matches);

        // 3. Filter good matches (Simple distance check)
        std::vector<cv::DMatch> goodMatches;
        std::vector<cv::Point3f> goodObjectPoints;
        std::vector<cv::Point2f> goodScenePoints;

        for (const auto &m : matches)
        {
            if (m.distance < 50.0f)
            { // Threshold
              // Crucial: We map the 3D point of the Reference to the 2D point of the Scene
                goodMatches.push_back(m);

                goodObjectPoints.push_back(refObjectPoints[m.queryIdx]);
                goodScenePoints.push_back(currKeypoints[m.trainIdx].pt);
            }
        }

        // 4. RANSAC & PnP
        // We need at least 4 points to solve PnP, but let's ask for 10 for stability
        if (goodScenePoints.size() < 10)
            return false;

        /*
    // Reference image corners (pixel coordinates)
    std::vector<cv::Point2f> refCorners = {
        cv::Point2f(0, 0),
        cv::Point2f(refImage.cols, 0),
        cv::Point2f(refImage.cols, refImage.rows),
        cv::Point2f(0, refImage.rows)};

    // Project corners to scene
    std::vector<cv::Point2f> sceneCorners;
    cv::perspectiveTransform(refCorners, sceneCorners, H);
    */

        // Draw the matches visually
        cv::Mat debugImg;
        // Draw only the good matches
        cv::drawMatches(refImage, refKeypoints, frame, currKeypoints,
                        goodMatches, debugImg,
                        cv::Scalar::all(-1), cv::Scalar::all(-1),
                        std::vector<char>(),
                        cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

        cv::imshow("Debug Matches", debugImg); // Creates a pop-up window
        cv::waitKey(1);

        // solvePnPRansac is robust against outliers (bad matches)
        cv::Mat inlierMask;
        bool success = cv::solvePnPRansac(goodObjectPoints, goodScenePoints, camMat, dist, rvec, tvec, false, 100, 8.0f, 0.99, inlierMask);

        if (success)
        {
            int inlierCount = cv::countNonZero(inlierMask);
            std::cout << "Total matches: " << goodMatches.size()
                      << " | RANSAC inliers: " << inlierCount << std::endl;

            // Reject if too few inliers
            if (inlierCount < 8)
            {
                return false;
            }
        }

        return success;
    }
};