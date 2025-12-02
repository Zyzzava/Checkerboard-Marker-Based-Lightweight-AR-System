#include "calibrator.hpp"
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

// Checkerboard pattern detection flags
const std::string kOutputDir = "calibration";
const std::string kWindowName = "Checkerboard Calibration";
const std::filesystem::path kBaseDataDir{"data"};

// Helper to convert cv::Mat to JSON array
nlohmann::json matToJson(const cv::Mat &mat)
{
    nlohmann::json rows = nlohmann::json::array();
    for (int i = 0; i < mat.rows; i++)
    {
        nlohmann::json cols = nlohmann::json::array();
        for (int j = 0; j < mat.cols; j++)
        {
            cols.push_back(mat.at<double>(i, j));
        }
        rows.push_back(cols);
    }
    return rows;
}

// Save calibration data to JSON file
void saveCalibrationData(const std::filesystem::path &dir,
                         const cv::Mat &cameraMatrix,
                         const cv::Mat &distCoeffs,
                         double reprojectionError)
{
    std::filesystem::create_directories(dir);
    nlohmann::json j;
    j["reprojection_error"] = reprojectionError;
    j["camera_matrix"] = matToJson(cameraMatrix);
    j["distortion_coefficients"] = matToJson(distCoeffs);

    std::ofstream out(dir / "calibration.json");
    if (out)
    {
        out << j.dump(4);
    }
}

// Helper to convert pattern size to string
std::string patternSizeToString(const cv::Size &patternSize)
{
    return std::to_string(patternSize.width) + "x" + std::to_string(patternSize.height);
}

// Draw calibration status on frame
void drawStatus(cv::Mat &frame, int saved, int kRequiredSamples)
{
    const std::string status = "Samples: " + std::to_string(saved) + " / " + std::to_string(kRequiredSamples);
    cv::putText(frame, status, {10, 25}, cv::FONT_HERSHEY_SIMPLEX, 0.7, {0, 255, 0}, 2);
    cv::putText(frame, "Space = save, ESC/q = cancel", {10, 55}, cv::FONT_HERSHEY_SIMPLEX, 0.6, {0, 255, 255}, 1);
}

// Main calibration function
void calibrateCamera(cv::VideoCapture &capture, int requiredSamples,
                     const std::string &outputDir, cv::Size patternSize, float squareSize)
{
    // Setup storage directories
    const std::filesystem::path storageDir = kBaseDataDir / outputDir / patternSizeToString(patternSize);
    const std::filesystem::path imageDir = storageDir / "images";
    // Create directories if they do not exist
    std::filesystem::create_directories(imageDir);

    capture.open(0);
    if (!capture.isOpened())
    {
        std::cerr << "Calibration requires an open camera.\n";
        return;
    }

    // calculate object points
    std::vector<cv::Point3f> templatePoints;
    for (int i = 0; i < patternSize.height; i++)
    {
        for (int j = 0; j < patternSize.width; j++)
        {
            templatePoints.push_back(cv::Point3f(j * squareSize, i * squareSize, 0));
        }
    }

    // Storage for captured points
    std::vector<cv::Point2f> imagePoints;
    // All points for all images
    std::vector<std::vector<cv::Point2f>> allImagePoints;
    // Corresponding object points
    std::vector<std::vector<cv::Point3f>> allObjectPoints;
    // Captured samples counter
    int collectedSamples = 0;

    // Create window
    cv::namedWindow(kWindowName, cv::WINDOW_AUTOSIZE);
    cv::Mat frame;
    cv::Mat gray;

    // Capture loop
    while (collectedSamples < requiredSamples)
    {
        capture >> frame;
        if (frame.empty())
        {
            continue;
        }

        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);

        const bool found = findChessboardCorners(gray, patternSize, imagePoints, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE);
        drawStatus(frame, collectedSamples, requiredSamples);

        cv::drawChessboardCorners(frame, patternSize, imagePoints, found);
        cv::imshow(kWindowName, frame);
        const int key = cv::waitKey(30);

        if (found && (key == 32)) // Space key
        {
            cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
            cv::cornerSubPix(gray, imagePoints, cv::Size(11, 11), cv::Size(-1, -1), criteria);

            allImagePoints.push_back(imagePoints);
            allObjectPoints.push_back(templatePoints);
            collectedSamples++;
            // Optionally save the calibration image
            cv::imwrite((imageDir / ("capture_" + std::to_string(collectedSamples) + ".png")).string(), frame);
        }

        if (key == 27 || key == 'q') // ESC or q
        {
            break;
        }
    }

    cv::destroyWindow(kWindowName);

    // Calibration logic would go here (not implemented for brevity)
    std::cout << "\nCalibrating camera... Please wait." << std::endl;
    cv::Mat cameraMatrix, distCoeffs, R, T;

    double reprojectionError = cv::calibrateCamera(allObjectPoints, allImagePoints, gray.size(), cameraMatrix, distCoeffs, R, T);

    std::cout << "Calibration finished!" << std::endl;
    std::cout << "Reprojection Error: " << reprojectionError << std::endl;
    std::cout << "Camera Matrix: \n"
              << cameraMatrix << std::endl;
    std::cout << "Distortion Coefficients: \n"
              << distCoeffs << std::endl;

    // Save calibration results
    saveCalibrationData(storageDir, cameraMatrix, distCoeffs, reprojectionError);
}

void captureReferenceImage(cv::VideoCapture &capture,
                           const std::string &outputDir)
{
    // Setup storage directories
    if (!outputDir.empty())
    {
        std::filesystem::create_directories(outputDir);
    }

    capture.open(0);
    if (!capture.isOpened())
    {
        std::cerr << "Reference image capture requires an open camera.\n";
        return;
    }
    std::string windowName = "Capture Reference Image";
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);

    cv::Mat frame;
    while (true)
    {
        capture >> frame;
        if (frame.empty())
        {
            continue;
        }

        cv::imshow(windowName, frame);
        const int key = cv::waitKey(30);

        if (key == 32) // Space key
        {
            std::filesystem::path outPath = std::filesystem::path(outputDir) / "reference.png";
            cv::imwrite(outPath.string(), frame);
            std::cout << "Reference image saved to " << outPath << std::endl;
            break;
        }

        if (key == 27 || key == 'q') // ESC or q
        {
            break;
        }
    }

    cv::destroyWindow(windowName);
}