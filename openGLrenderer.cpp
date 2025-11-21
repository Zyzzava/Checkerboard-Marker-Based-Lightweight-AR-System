#include "openGLrenderer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

// Helper function implementations (compileShader, createShaderProgram)
// These read shader files and compile them
GLuint Renderer::compileShader(const char *filepath, GLenum type)
{
    // Read shader source from file
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << filepath << std::endl;
        return 0;
    }
    // Read file contents into a string
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    const char *src_c_str = source.c_str();

    // Compile shader
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src_c_str, NULL);
    glCompileShader(shader);

    // Check for compilation errors
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Error compiling shader (" << filepath << "): " << infoLog << std::endl;
        return 0;
    }

    return shader;
}

GLuint Renderer::createShaderProgram(const char *vertPath, const char *fragPath)
{
    // Compile vertex and fragment shaders
    GLuint vertexShader = compileShader(vertPath, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragPath, GL_FRAGMENT_SHADER);
    if (vertexShader == 0 || fragmentShader == 0)
        return 0;

    // Link shaders into a program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check for linking errors
    GLint success;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Error linking shader program: " << infoLog << std::endl;
        return 0;
    }
    // Clean up shaders as they're linked into the program now
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Use the macro defined in CMake
std::string shaderDir = std::string(PROJECT_ROOT) + "/data/shaders/";

Renderer::Renderer(int width, int height) : screenWidth(width), screenHeight(height)
{
    // -- SETUP FOR BACKGROUND RENDERING --
    backgroundShader = createShaderProgram((shaderDir + "background.vert").c_str(), (shaderDir + "background.frag").c_str());
    float quadVertices[] = {
        // x, y, u, v
        -1.0f,
        -1.0f,
        0.0f,
        0.0f,
        1.0f,
        -1.0f,
        1.0f,
        0.0f,
        -1.0f,
        1.0f,
        0.0f,
        1.0f,
        1.0f,
        1.0f,
        1.0f,
        1.0f};

    glGenVertexArrays(1, &backgroundVAO);
    glGenBuffers(1, &backgroundVBO);
    glBindVertexArray(backgroundVAO);
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);

    // FIX: Set up Attribute 0 (Position: x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // FIX: Set up Attribute 1 (TexCoord: u, v)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glGenTextures(1, &cameraTexture);
    glBindTexture(GL_TEXTURE_2D, cameraTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    // -- SETUP FOR CUBE RENDERING --
    cubeShader = createShaderProgram((shaderDir + "cube.vert").c_str(), (shaderDir + "cube.frag").c_str());
    float cubeVertices[] = {
        // positions      // colors
        -25.f, -25.f, -25.f, 1.f, 0.f, 0.f,
        25.f, -25.f, -25.f, 1.f, 0.f, 0.f,
        25.f, 25.f, -25.f, 1.f, 0.f, 0.f,
        25.f, 25.f, -25.f, 1.f, 0.f, 0.f,
        -25.f, 25.f, -25.f, 1.f, 0.f, 0.f,
        -25.f, -25.f, -25.f, 1.f, 0.f, 0.f,
        // all 36 vertices for a cube here with colors
        -25.f, -25.f, 25.f, 0.f, 1.f, 0.f,
        25.f, -25.f, 25.f, 0.f, 1.f, 0.f,
        25.f, 25.f, 25.f, 0.f, 1.f, 0.f,
        25.f, 25.f, 25.f, 0.f, 1.f, 0.f,
        -25.f, 25.f, 25.f, 0.f, 1.f, 0.f,
        -25.f, -25.f, 25.f, 0.f, 1.f, 0.f,

        -25.f, 25.f, 25.f, 0.f, 0.f, 1.f,
        -25.f, 25.f, -25.f, 0.f, 0.f, 1.f,
        -25.f, -25.f, -25.f, 0.f, 0.f, 1.f,
        -25.f, -25.f, -25.f, 0.f, 0.f, 1.f,
        -25.f, -25.f, 25.f, 0.f, 0.f, 1.f,
        -25.f, 25.f, 25.f, 0.f, 0.f, 1.f,

        25.f, 25.f, 25.f, 1.f, 1.f, 0.f,
        25.f, 25.f, -25.f, 1.f, 1.f, 0.f,
        25.f, -25.f, -25.f, 1.f, 1.f, 0.f,
        25.f, -25.f, -25.f, 1.f, 1.f, 0.f,
        25.f, -25.f, 25.f, 1.f, 1.f, 0.f,
        25.f, 25.f, 25.f, 1.f, 1.f, 0.f,

        -25.f, -25.f, -25.f, 1.f, 0.f, 1.f,
        25.f, -25.f, -25.f, 1.f, 0.f, 1.f,
        25.f, -25.f, 25.f, 1.f, 0.f, 1.f,
        25.f, -25.f, 25.f, 1.f, 0.f, 1.f,
        -25.f, -25.f, 25.f, 1.f, 0.f, 1.f,
        -25.f, -25.f, -25.f, 1.f, 0.f, 1.f,

        -25.f, 25.f, -25.f, 0.f, 1.f, 1.f,
        25.f, 25.f, -25.f, 0.f, 1.f, 1.f,
        25.f, 25.f, 25.f, 0.f, 1.f, 1.f,
        25.f, 25.f, 25.f, 0.f, 1.f, 1.f,
        -25.f, 25.f, 25.f, 0.f, 1.f, 1.f,
        -25.f, 25.f, -25.f, 0.f, 1.f, 1.f};
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
}

Renderer::~Renderer()
{
    glDeleteVertexArrays(1, &backgroundVAO);
    glDeleteBuffers(1, &backgroundVBO);
    glDeleteProgram(backgroundShader);
    glDeleteTextures(1, &cameraTexture);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVBO);
    glDeleteProgram(cubeShader);
}

// Update the background texture with a new camera frame
void Renderer::updateBackground(const cv::Mat &frame)
{
    // Convert BGR to RGB
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);

    cv::flip(rgb, rgb, 0); // Flip vertically for OpenGL

    glBindTexture(GL_TEXTURE_2D, cameraTexture);
    // Update texture with new frame data
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, screenWidth, screenHeight, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);
}

// Draw the background quad with the camera texture
// Difference between this and updateBackground is that this actually renders it
// While updateBackground just updates the texture data
void Renderer::drawBackground()
{
    // Disable depth test for background
    glDisable(GL_DEPTH_TEST);
    // Use the background shader program
    glUseProgram(backgroundShader);
    // Bind the background VAO and texture
    glBindVertexArray(backgroundVAO);
    // Bind the camera texture
    glBindTexture(GL_TEXTURE_2D, cameraTexture);
    // Draw the quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

// Draw the cube with given modelview and projection matrices
void Renderer::drawCube(const GLfloat *modelViewMatrix, const GLfloat *projectionMatrix)
{
    static int debugFrameCounter = 0;
    if (debugFrameCounter++ % 60 == 0)
    {
        std::cout << "\n--- Debug Frame " << debugFrameCounter << " ---" << std::endl;

        std::cout << "Projection Matrix (Column-Major):" << std::endl;
        for (int i = 0; i < 4; i++)
        {
            std::cout << "[ ";
            for (int j = 0; j < 4; j++)
                std::cout << projectionMatrix[i + j * 4] << "\t";
            std::cout << "]" << std::endl;
        }

        std::cout << "ModelView Matrix (Column-Major):" << std::endl;
        for (int i = 0; i < 4; i++)
        {
            std::cout << "[ ";
            for (int j = 0; j < 4; j++)
                std::cout << modelViewMatrix[i + j * 4] << "\t";
            std::cout << "]" << std::endl;
        }

        // Specific check for Translation (last column of ModelView)
        float tx = modelViewMatrix[12];
        float ty = modelViewMatrix[13];
        float tz = modelViewMatrix[14];
        std::cout << "Translation (Tvec): " << tx << ", " << ty << ", " << tz << std::endl;
    }

    glEnable(GL_DEPTH_TEST);
    glUseProgram(cubeShader);

    // Compute Model-View-Projection (MVP) matrix
    GLfloat mvpMatrix[16];
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            mvpMatrix[i + j * 4] = 0.0f;
            for (int k = 0; k < 4; k++)
            {
                mvpMatrix[i + j * 4] += projectionMatrix[i + k * 4] * modelViewMatrix[k + j * 4];
            }
        }
    }

    // Get uniform location for MVP
    GLint mvpLocation = glGetUniformLocation(cubeShader, "mvp");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvpMatrix);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::buildProjectionMatrix(const cv::Mat &cameraMatrix, int screen_w, int screen_h, GLfloat *projectionMatrix)
{
    float near = 0.1f;   // Near clipping plane
    float far = 3000.0f; // Far clipping plane

    // Extract focal lengths and principal point from camera matrix
    // focal length in x direction
    float fx = static_cast<float>(cameraMatrix.at<double>(0, 0));
    // focal length in y direction
    float fy = static_cast<float>(cameraMatrix.at<double>(1, 1));
    // principal point (optical center) coordinates
    float cx = static_cast<float>(cameraMatrix.at<double>(0, 2));
    // principal point (optical center) coordinates
    float cy = static_cast<float>(cameraMatrix.at<double>(1, 2));

    // zero out the memory
    std::memset(projectionMatrix, 0, 16 * sizeof(GLfloat));

    // 1. Scale X based on focal length
    projectionMatrix[0] = 2.0f * fx / screen_w;
    // 2. Scale Y based on focal length
    projectionMatrix[5] = 2.0f * fy / screen_h;

    // 3. Center offset (Principal Point)
    // Maps the camera's optical center to the screen center
    projectionMatrix[8] = 2.0f * (cx / screen_w) - 1.0f;
    projectionMatrix[9] = 2.0f * (cy / screen_h) - 1.0f;

    // 4. Depth mapping (Standard OpenGL Perspective)
    projectionMatrix[10] = -(far + near) / (far - near);
    projectionMatrix[11] = -1.0f;
    projectionMatrix[14] = -(2.0f * far * near) / (far - near);

    // 5. Homogeneous coordinate
    projectionMatrix[15] = 0.0f;
}