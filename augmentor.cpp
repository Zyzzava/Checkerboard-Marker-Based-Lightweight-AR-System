#include "augmentor.hpp"
#include "openGLrenderer.hpp"
#include "jsonHelper.hpp"
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include "tracker.hpp"
#include "chessboard_tracker.hpp"
#include "nft_tracker.hpp"
#include "statistics.hpp"
#include <fstream>

void augmentLoop(cv::VideoCapture &capture, bool &useNft, cv::Size patternSize, float squareSize, const std::string &experimentName, const std::string &testName)
{
    std::unique_ptr<PoseTracker> tracker;

    if (useNft)
    {
        auto nft = std::make_unique<NFTTracker>("data/reference/reference.png");
        nft->init();
        tracker = std::move(nft);
    }
    else
    {
        auto chess = std::make_unique<ChessboardTracker>(patternSize, squareSize);
        chess->init();
        tracker = std::move(chess);
    }

    // load calibration data
    cv::Mat cameraMatrix, distCoeffs;
    initAugmentor(cameraMatrix, distCoeffs, patternSize);

    // Let's print it for debugging
    std::cout << "Camera Matrix: " << cameraMatrix << std::endl;
    std::cout << "Distortion Coefficients: " << distCoeffs << std::endl;

    if (cameraMatrix.empty() || distCoeffs.empty())
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
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));
    std::cout << "Camera capture size: " << frame_width << "x" << frame_height << std::endl;

    // Fail-safe debugging for frame_width and frame_height
    if (frame_width <= 0 || frame_height <= 0)
    {
        std::cerr << "Warning: Invalid frame dimensions." << std::endl;
    }

    // initialize OpenGL window
    if (!glfwInit())
        return;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Required on Mac
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    GLFWwindow *window = glfwCreateWindow(frame_width, frame_height, "AR", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return;
    }
    glfwMakeContextCurrent(window);
    if (glewInit() != GLEW_OK)
        return;

    // create renderer
    Renderer renderer(frame_width, frame_height);

    // calculate projection matrix
    GLfloat projectionMatrix[16];
    renderer.buildProjectionMatrix(cameraMatrix, frame_width, frame_height, projectionMatrix);

    cv::Mat frame;
    cv::Mat rvec, tvec, rotationMatrix;

    // Statistical collection
    int frameCount = 0;
    int setCount = 0;
    const int framesPerSet = 800;
    SessionStats stats;
    auto t_start = std::chrono::high_resolution_clock::now();

    while (!glfwWindowShouldClose(window))
    {
        auto frameStart = std::chrono::high_resolution_clock::now();
        // capture frame
        cap >> frame;
        if (frame.empty())
        {
            std::cerr << "Error: Could not read frame." << std::endl;
            break;
        }
        // 1. Undistort frame
        cv::Mat undistortedFrame;
        cv::undistort(frame, undistortedFrame, cameraMatrix, distCoeffs);
        frame = undistortedFrame;

        // 2. Create a "Zero" distortion vector for tracking
        // Since the frame is now undistorted, we treat it as a perfect pinhole camera
        cv::Mat zeroDist = cv::Mat::zeros(4, 1, cv::DataType<double>::type);

        // Update viewport in case window size != framebuffer size
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        // display camera frame as background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // update and draw camera frame as background
        renderer.updateBackground(frame);
        renderer.drawBackground();

        // 3. Estimate pose using zeroDist
        // We use the original cameraMatrix (approximation), but NO distortion
        bool success = tracker->estimatePose(frame, cameraMatrix, zeroDist, rvec, tvec);

        if (success)
        {
            cv::Rodrigues(rvec, rotationMatrix);

            // build modelview matrix
            double modelViewMatrix[16];
            std::memset(modelViewMatrix, 0, sizeof(modelViewMatrix));

            // Column 0
            modelViewMatrix[0] = (double)rotationMatrix.at<double>(0, 0);
            modelViewMatrix[1] = -(double)rotationMatrix.at<double>(1, 0);
            modelViewMatrix[2] = -(double)rotationMatrix.at<double>(2, 0);
            modelViewMatrix[3] = 0.0;

            // Column 1  (flip Y)
            modelViewMatrix[4] = (double)rotationMatrix.at<double>(0, 1);
            modelViewMatrix[5] = -(double)rotationMatrix.at<double>(1, 1);
            modelViewMatrix[6] = -(double)rotationMatrix.at<double>(2, 1);
            modelViewMatrix[7] = 0.0;

            // Column 2  (flip Z)
            modelViewMatrix[8] = (double)rotationMatrix.at<double>(0, 2);
            modelViewMatrix[9] = -(double)rotationMatrix.at<double>(1, 2);
            modelViewMatrix[10] = -(double)rotationMatrix.at<double>(2, 2);
            modelViewMatrix[11] = 0.0;

            // Column 3 = translation (flip Y and Z)
            modelViewMatrix[12] = (double)tvec.at<double>(0);
            modelViewMatrix[13] = -(double)tvec.at<double>(1);
            modelViewMatrix[14] = -(double)tvec.at<double>(2);
            modelViewMatrix[15] = 1.0;

            // --- b. RENDER (PROJECT POINTS) ---
            // For a simple test, let's project the 3D axes onto the image.
            std::vector<cv::Point3f> axisPoints;
            axisPoints.push_back(cv::Point3f(0, 0, 0));               // Origin (for the circle)
            axisPoints.push_back(cv::Point3f(squareSize * 3, 0, 0));  // X-axis
            axisPoints.push_back(cv::Point3f(0, squareSize * 3, 0));  // Y-axis
            axisPoints.push_back(cv::Point3f(0, 0, -squareSize * 3)); // Z-axis

            std::vector<cv::Point2f> image_axes;
            // Use zeroDist, so lines match with OpenGL render
            cv::projectPoints(axisPoints, rvec, tvec, cameraMatrix, distCoeffs, image_axes);

            // Draw the projected axes on the image
            cv::line(frame, image_axes[0], image_axes[1], cv::Scalar(0, 0, 255), 3); // X-axis in Red
            cv::line(frame, image_axes[0], image_axes[2], cv::Scalar(0, 255, 0), 3); // Y-axis in Green
            cv::line(frame, image_axes[0], image_axes[3], cv::Scalar(255, 0, 0), 3); // Z-axis in Blue

            // render virtual object
            renderer.drawCube(modelViewMatrix, projectionMatrix);
        }

        // Statistical collection
        auto frameEnd = std::chrono::high_resolution_clock::now();
        double frameTimeMs = std::chrono::duration<double, std::milli>(frameEnd - frameStart).count();

        stats.frames.push_back(FrameStats{
            frameCount,
            std::chrono::duration<double>(frameEnd - t_start).count(),
            success,
            rvec.clone(), tvec.clone(),
            frameTimeMs});

        // Increment frame count
        frameCount++;

        // DEBUGGING
        if (!useNft)
        {
            ChessboardTracker *chess = dynamic_cast<ChessboardTracker *>(tracker.get());
            if (chess && chess->lastCorners.size() == patternSize.width * patternSize.height)
            {
                cv::drawChessboardCorners(frame, patternSize, chess->lastCorners, true);
            }
        }

        // Draw frame count on the image
        std::string frameText = "Frame: " + std::to_string(frameCount);
        cv::putText(
            frame,
            frameText,
            cv::Point(20, 40), // position (x, y)
            cv::FONT_HERSHEY_SIMPLEX,
            1.0,                   // font scale
            cv::Scalar(0, 255, 0), // color (green)
            2                      // thickness
        );

        // ESCAPE WINDOW (Press ESC to exit)
        cv::imshow("AR View", frame);
        if (cv::waitKey(1) == 27)
        { // ESC key
            break;
        }

        // Only limit to 800 frames if experiment is "pose_stability"
        if (experimentName == "pose_stability" && frameCount >= 800)
        {
            std::cout << "Reached 800 frames for pose_stability, exiting augmentation loop." << std::endl;
            break;
        }

        // For detection_robustness
        if (experimentName == "detection_robustness" && frameCount >= framesPerSet)
        {
            // Build file path for this test
            std::string statsPath;
            if (useNft)
                statsPath = "data/statistics/NFT/" + experimentName + "/session_stats_nft_" + testName + ".json";
            else
                statsPath = "data/statistics/Checkerboard/" + experimentName + "/session_stats_checkerboard_" + testName + ".json";

            std::filesystem::create_directories(std::filesystem::path(statsPath).parent_path());
            std::ofstream out(statsPath);
            if (out.is_open())
            {
                out << stats.toJson().dump(4);
                out.close();
                std::cout << "Session statistics for test '" << testName << "' saved to " << statsPath << std::endl;
            }
            else
            {
                std::cerr << "Unable to open file to save session statistics for test '" << testName << "'." << std::endl;
            }
            std::cout << "Completed 800 frames for detection_robustness (" << testName << "), exiting augmentation loop." << std::endl;
            break;
        }

        // swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // cleanup
    glfwDestroyWindow(window);
    glfwTerminate();

    // Only save at the end if testName is empty (i.e., not a detection_robustness test)
    if (testName.empty())
    {
        std::string statsPath;
        if (useNft)
            statsPath = "data/statistics/NFT/" + experimentName + "/session_stats_nft.json";
        else
            statsPath = "data/statistics/Checkerboard/" + experimentName + "/session_stats_checkerboard.json";

        std::filesystem::create_directories(std::filesystem::path(statsPath).parent_path());
        std::ofstream out(statsPath);
        if (out.is_open())
        {
            out << stats.toJson().dump(4);
            out.close();
            std::cout << "Session statistics saved to " << statsPath << std::endl;
        }
        else
        {
            std::cerr << "Unable to open file to save session statistics." << std::endl;
        }
    }
}

void initAugmentor(cv::Mat &cameraMatrix, cv::Mat &distCoeffs, cv::Size patternSize)
{
    std::string patternStr = std::to_string(patternSize.width) + "x" + std::to_string(patternSize.height);
    std::filesystem::path calibrationJson = std::filesystem::path("data/calibration") / patternStr / "calibration.json";
    if (!ar::loadCalibrationData(calibrationJson, cameraMatrix, distCoeffs))
    {
        std::cerr << "Unable to read " << calibrationJson << std::endl;
    }
}