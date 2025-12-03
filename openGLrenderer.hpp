#pragma once
#include <GL/glew.h>
#include <opencv2/opencv.hpp>

// OpenGL Renderer for AR application
class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer();

    // Update the background texture with the latest camera frame
    void updateBackground(const cv::Mat &frame);
    // Draw the background quad with the camera texture
    void drawBackground();
    // Draw the cube with given modelview and projection matrices
    void drawCube(const double *modelViewMatrix, const GLfloat *projectionMatrix);
    // Build projection matrix from camera intrinsics
    void buildProjectionMatrix(const cv::Mat &cameraMatrix, int screen_w, int screen_h, GLfloat *projectionMatrix);

private:
    // Helper to compile shaders
    GLuint compileShader(const char *filepath, GLenum type);
    // Helper to create shader program from vertex and fragment shaders
    GLuint createShaderProgram(const char *vertPath, const char *fragPath);

    // Background rendering resources
    GLuint backgroundVAO, backgroundVBO; // Vertex Array Object and Vertex Buffer Object
    GLuint backgroundShader;             // Shader program for background
    GLuint cameraTexture;                // Texture for camera frame

    // Cube rendering resources
    GLuint cubeVAO, cubeVBO; // Vertex Array Object and Vertex Buffer Object for cube
    GLuint cubeShader;       // Shader program for cube

    int screenWidth, screenHeight; // Screen dimensions
};