#include "augmentor.hpp"
#include "openGLrenderer.hpp"
#include "jsonHelper.hpp"
#include <filesystem>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

namespace
{
    const std::filesystem::path kCalibrationJson{"data/calibration/calibration.json"};
}

void augmentLoop(cv::VideoCapture &capture)
{
    // load calibration data
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
    int frame_width = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_WIDTH));
    int frame_height = static_cast<int>(cap.get(cv::CAP_PROP_FRAME_HEIGHT));

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
    cv::Size patternSize(8, 6);

    while (!glfwWindowShouldClose(window))
    {
        // capture frame
        cap >> frame;
        if (frame.empty())
        {
            std::cerr << "Error: Could not read frame." << std::endl;
            break;
        }
        // display camera frame as background
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // update and draw camera frame as background
        renderer.updateBackground(frame);
        renderer.drawBackground();
        

        // estimate pose
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        std::vector<cv::Point2f> corner_pts;
        bool success = cv::findChessboardCorners(gray, patternSize, corner_pts, cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_FAST_CHECK | cv::CALIB_CB_NORMALIZE_IMAGE);

                if (success)
        {
            // Refine corner locations to sub-pixel accuracy
            cv::TermCriteria criteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001);
            cv::cornerSubPix(gray, corner_pts, cv::Size(11, 11), cv::Size(-1, -1), criteria);

            cv::Mat rvec, tvec, rotationMatrix;
            cv::solvePnP(objectPoints, corner_pts, cameraMatrix, distCoeffs, rvec, tvec);
            cv::Rodrigues(rvec, rotationMatrix);

            // build modelview matrix
            GLfloat modelViewMatrix[16] = {
                (float)rotationMatrix.at<double>(0, 0), (float)-rotationMatrix.at<double>(1, 0), (float)-rotationMatrix.at<double>(2, 0), 0.0f,
                (float)rotationMatrix.at<double>(0, 1), (float)-rotationMatrix.at<double>(1, 1), (float)-rotationMatrix.at<double>(2, 1), 0.0f,
                (float)rotationMatrix.at<double>(0, 2), (float)-rotationMatrix.at<double>(1, 2), (float)-rotationMatrix.at<double>(2, 2), 0.0f,
                (float)tvec.at<double>(0), (float)-tvec.at<double>(1), (float)-tvec.at<double>(2), 1.0f};
            // render virtual object
            renderer.drawCube(modelViewMatrix, projectionMatrix);
        }
        // swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // cleanup
    glfwDestroyWindow(window);
    glfwTerminate();
}

void initAugmentor(cv::Mat &cameraMatrix, cv::Mat &distCoeffs, std::vector<cv::Point3f> &objectPoints)
{
    if (!ar::loadCalibrationData(kCalibrationJson, cameraMatrix, distCoeffs, objectPoints))
    {
        std::cerr << "Unable to read " << kCalibrationJson << std::endl;
    }
}