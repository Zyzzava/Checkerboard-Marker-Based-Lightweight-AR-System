#include "tracker.hpp"
#include <opencv2/features2d.hpp>
#include <iostream>

// Implements pose estimation using Natural Feature Tracking (NFT)
class NFTTracker : public PoseTracker
{
    std::string imagePath;                    // Path to reference image
    cv::Mat refImage;                         // Reference image
    cv::Mat refDescriptors;                   // Descriptors of reference image keypoints
    std::vector<cv::KeyPoint> refKeypoints;   // Keypoints of reference image
    std::vector<cv::Point3f> refObjectPoints; // The "3D" representation of the image pixels

    cv::Ptr<cv::Feature2D> detector;        // Feature detector (ORB)
    cv::Ptr<cv::DescriptorMatcher> matcher; // Descriptor matcher

    // Scale factor to convert pixels to "World Units"
    float scaleFactor = 0.1f;

public:
    NFTTracker(std::string path) : imagePath(path) {}

    void init() override
    {
        // Load Reference Image
        refImage = cv::imread(imagePath, cv::IMREAD_GRAYSCALE);
        if (refImage.empty())
        {
            std::cerr << "Could not load reference image!" << std::endl;
            return;
        }

        // Setup ORB Detector
        detector = cv::ORB::create(5000); // Track 5000 features
        matcher = cv::DescriptorMatcher::create("BruteForce-Hamming");

        // Analyze the Reference Image
        detector->detectAndCompute(refImage, cv::noArray(), refKeypoints, refDescriptors);

        // Create 3D Object Points from 2D Keypoints
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
        // convert to grayscale
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        // Detect features in current frame
        std::vector<cv::KeyPoint> currKeypoints;
        // Compute descriptors
        cv::Mat currDescriptors;
        detector->detectAndCompute(gray, cv::noArray(), currKeypoints, currDescriptors);

        if (currDescriptors.empty())
            return false;

        // Match against reference
        std::vector<std::vector<cv::DMatch>> knn_matches;
        matcher->knnMatch(refDescriptors, currDescriptors, knn_matches, 2);

        // Filter good matches (Simple distance check)
        std::vector<cv::DMatch> goodMatches;
        std::vector<cv::Point3f> goodObjectPoints;
        std::vector<cv::Point2f> goodScenePoints;

        const float ratio_thresh = 0.75f; // Lowe's ratio test
        for (const auto &match_pair : knn_matches)
        {
            if (match_pair.size() == 2 && match_pair[0].distance < ratio_thresh * match_pair[1].distance)
            {
                // map the 3D point of the Reference to the 2D point of the Scene
                goodMatches.push_back(match_pair[0]);
                goodObjectPoints.push_back(refObjectPoints[match_pair[0].queryIdx]);
                goodScenePoints.push_back(currKeypoints[match_pair[0].trainIdx].pt);
            }
        }

        // RANSAC & PnP
        // We need at least 4 points to solve PnP, but ask for 10 for stability
        if (goodScenePoints.size() < 10)
            return false;

        // Draw the matches visually
        drawMatches(refImage, refKeypoints, frame, currKeypoints, goodMatches);

        // solvePnPRansac is robust against outliers
        // It will return the inliers used for the final pose estimation
        cv::Mat inlierMask;
        bool success = cv::solvePnPRansac(goodObjectPoints, goodScenePoints, camMat, dist, rvec, tvec, false, 100, 8.0f, 0.99, inlierMask);

        int inlierCount = cv::countNonZero(inlierMask);

        if (success)
        {
            // Reject if too few inliers
            if (inlierCount < 8)
            {
                return false;
            }
        }

        return success;
    }

    void drawMatches(const cv::Mat &refImage,
                     const std::vector<cv::KeyPoint> &refKeypoints,
                     const cv::Mat &frame,
                     const std::vector<cv::KeyPoint> &currKeypoints,
                     const std::vector<cv::DMatch> &goodMatches)
    {
        cv::Mat debugImg;
        // Draw only the good matches
        cv::drawMatches(refImage, refKeypoints, frame, currKeypoints,
                        goodMatches, debugImg,
                        cv::Scalar::all(-1), cv::Scalar::all(-1),
                        std::vector<char>(),
                        cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

        cv::imshow("Debug Matches", debugImg); // Creates a pop-up window
        cv::waitKey(1);
    }
};
