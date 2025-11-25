#pragma once
#include <GL/glew.h>
#include <opencv2/opencv.hpp>

class Renderer
{
public:
    Renderer(int width, int height);
    ~Renderer();

    void updateBackground(const cv::Mat &frame);
    void drawBackground();
    void drawCube(const double *modelViewMatrix, const GLfloat *projectionMatrix);
    void buildProjectionMatrix(const cv::Mat &cameraMatrix, int screen_w, int screen_h, GLfloat *projectionMatrix);

private:
    // Helper to compile shaders
    GLuint compileShader(const char *filepath, GLenum type);
    GLuint createShaderProgram(const char *vertPath, const char *fragPath);

    // Background rendering resources
    GLuint backgroundVAO, backgroundVBO;
    GLuint backgroundShader;
    GLuint cameraTexture;

    // Cube rendering resources
    GLuint cubeVAO, cubeVBO;
    GLuint cubeShader;

    int screenWidth, screenHeight;
};