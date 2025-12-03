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
    GLuint shaderProgram = glCreateProgram();      // Create program
    glAttachShader(shaderProgram, vertexShader);   // Attach vertex shader
    glAttachShader(shaderProgram, fragmentShader); // Attach fragment shader
    glLinkProgram(shaderProgram);                  // Link program

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
    glDeleteShader(vertexShader);   // Delete vertex shader
    glDeleteShader(fragmentShader); // Delete fragment shader

    return shaderProgram;
}

// Use the macro defined in CMake
std::string shaderDir = std::string(PROJECT_ROOT) + "/shaders/";

Renderer::Renderer(int width, int height) : screenWidth(width), screenHeight(height)
{
    // -- SETUP FOR BACKGROUND RENDERING --
    // Load and compile background shaders
    backgroundShader = createShaderProgram((shaderDir + "background.vert").c_str(), (shaderDir + "background.frag").c_str());
    // Fullscreen quad vertices (x, y, u, v)
    float quadVertices[] = {
        // x, y, u, v
        -1.0f, -1.0f, 0.0f, 0.0f, // Bottom-left
        1.0f, -1.0f, 1.0f, 0.0f,  // Bottom-right
        -1.0f, 1.0f, 0.0f, 1.0f,  // Top-left
        1.0f, 1.0f, 1.0f, 1.0f};  // Top-right

    glGenVertexArrays(1, &backgroundVAO);                                               // Generate VAO
    glGenBuffers(1, &backgroundVBO);                                                    // Generate VBO
    glBindVertexArray(backgroundVAO);                                                   // Bind VAO
    glBindBuffer(GL_ARRAY_BUFFER, backgroundVBO);                                       // Bind VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW); // Upload vertex data

    // Attribute 0 (Position: x, y)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0); // position
    glEnableVertexAttribArray(0);                                                  // Enable attribute 0

    // Attribute 1 (TexCoord: u, v)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float))); // texcoord
    glEnableVertexAttribArray(1);                                                                    // Enable attribute 1

    glGenTextures(1, &cameraTexture);                                                                     // Generate texture for camera frame
    glBindTexture(GL_TEXTURE_2D, cameraTexture);                                                          // Bind texture
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);                                     // Set texture parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);                                     // Set texture parameters
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, screenWidth, screenHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL); // Allocate texture

    // -- SETUP FOR CUBE RENDERING --
    // Load and compile cube shaders
    cubeShader = createShaderProgram((shaderDir + "cube.vert").c_str(), (shaderDir + "cube.frag").c_str());
    // Cube vertices (x, y, z, r, g, b)
    float cubeVertices[] = {
        // positions            // colors
        // Dimensions: Width 25, Height 25, Depth 25.
        // Z range: 0.0f (base) to -25.0f (top)

        // BACK FACE (Top of cube, z = -25.0f)
        -12.5f, -12.5f, -25.0f, 1.0f, 0.0f, 0.0f,
        12.5f, -12.5f, -25.0f, 1.0f, 0.0f, 0.0f,
        12.5f, 12.5f, -25.0f, 1.0f, 0.0f, 0.0f,
        12.5f, 12.5f, -25.0f, 1.0f, 0.0f, 0.0f,
        -12.5f, 12.5f, -25.0f, 1.0f, 0.0f, 0.0f,
        -12.5f, -12.5f, -25.0f, 1.0f, 0.0f, 0.0f,

        // FRONT FACE (Base of cube, touching board, z = 0.0f)
        -12.5f, -12.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        12.5f, -12.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        12.5f, 12.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        12.5f, 12.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -12.5f, 12.5f, 0.0f, 0.0f, 1.0f, 0.0f,
        -12.5f, -12.5f, 0.0f, 0.0f, 1.0f, 0.0f,

        // LEFT FACE
        -12.5f, 12.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        -12.5f, 12.5f, -25.0f, 0.0f, 0.0f, 1.0f,
        -12.5f, -12.5f, -25.0f, 0.0f, 0.0f, 1.0f,
        -12.5f, -12.5f, -25.0f, 0.0f, 0.0f, 1.0f,
        -12.5f, -12.5f, 0.0f, 0.0f, 0.0f, 1.0f,
        -12.5f, 12.5f, 0.0f, 0.0f, 0.0f, 1.0f,

        // RIGHT FACE
        12.5f, 12.5f, 0.0f, 1.0f, 1.0f, 0.0f,
        12.5f, 12.5f, -25.0f, 1.0f, 1.0f, 0.0f,
        12.5f, -12.5f, -25.0f, 1.0f, 1.0f, 0.0f,
        12.5f, -12.5f, -25.0f, 1.0f, 1.0f, 0.0f,
        12.5f, -12.5f, 0.0f, 1.0f, 1.0f, 0.0f,
        12.5f, 12.5f, 0.0f, 1.0f, 1.0f, 0.0f,

        // BOTTOM FACE
        -12.5f, -12.5f, -25.0f, 1.0f, 0.0f, 1.0f,
        12.5f, -12.5f, -25.0f, 1.0f, 0.0f, 1.0f,
        12.5f, -12.5f, 0.0f, 1.0f, 0.0f, 1.0f,
        12.5f, -12.5f, 0.0f, 1.0f, 0.0f, 1.0f,
        -12.5f, -12.5f, 0.0f, 1.0f, 0.0f, 1.0f,
        -12.5f, -12.5f, -25.0f, 1.0f, 0.0f, 1.0f,

        // TOP FACE
        -12.5f, 12.5f, -25.0f, 0.0f, 1.0f, 1.0f,
        12.5f, 12.5f, -25.0f, 0.0f, 1.0f, 1.0f,
        12.5f, 12.5f, 0.0f, 0.0f, 1.0f, 1.0f,
        12.5f, 12.5f, 0.0f, 0.0f, 1.0f, 1.0f,
        -12.5f, 12.5f, 0.0f, 0.0f, 1.0f, 1.0f,
        -12.5f, 12.5f, -25.0f, 0.0f, 1.0f, 1.0f};
    glGenVertexArrays(1, &cubeVAO);                                                    // Generate VAO
    glGenBuffers(1, &cubeVBO);                                                         // Generate VBO
    glBindVertexArray(cubeVAO);                                                        // Bind VAO
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);                                            // Bind VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW); // Upload vertex data

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0); // position
    glEnableVertexAttribArray(0);                                                  // Enable attribute 0
    // Color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float))); // color
    glEnableVertexAttribArray(1);                                                                    // Enable attribute 1

    // Enable depth testing for 3D cube rendering
    glEnable(GL_DEPTH_TEST);
}

// Destructor to clean up OpenGL resources
Renderer::~Renderer()
{
    // Clean up OpenGL resources
    glDeleteVertexArrays(1, &backgroundVAO); // Delete background VAO
    glDeleteBuffers(1, &backgroundVBO);      // Delete background VBO
    glDeleteProgram(backgroundShader);       // Delete background shader program
    glDeleteTextures(1, &cameraTexture);     // Delete camera texture
    glDeleteVertexArrays(1, &cubeVAO);       // Delete cube VAO
    glDeleteBuffers(1, &cubeVBO);            // Delete cube VBO
    glDeleteProgram(cubeShader);             // Delete cube shader program
}

// Update the background texture with a new camera frame
void Renderer::updateBackground(const cv::Mat &frame)
{
    // Convert BGR to RGB
    cv::Mat rgb;
    // Convert the color space from BGR to RGB
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    // Flip the image vertically to match OpenGL's coordinate system
    cv::flip(rgb, rgb, 0); // Flip vertically for OpenGL

    // Bind the camera texture
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
void Renderer::drawCube(const double *modelViewMatrix, const GLfloat *projectionMatrix)
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
        double tx = modelViewMatrix[12]; // X translation
        double ty = modelViewMatrix[13]; // Y translation
        double tz = modelViewMatrix[14]; // Z translation
        std::cout << "Translation (Tvec): " << tx << ", " << ty << ", " << tz << std::endl;
    }

    glEnable(GL_DEPTH_TEST);  // Enable depth test for cube rendering
    glUseProgram(cubeShader); // Use the cube shader program

    // Use double precision for all matrix math
    double modelViewMatrixD[16];  // Copy input float matrices to double
    double projectionMatrixD[16]; // Copy input float matrices to double
    // Copy input float matrices to double
    for (int i = 0; i < 16; ++i)
    {
        modelViewMatrixD[i] = static_cast<double>(modelViewMatrix[i]);   // Copy modelViewMatrix
        projectionMatrixD[i] = static_cast<double>(projectionMatrix[i]); // Copy projectionMatrix
    }

    // Compute Model-View-Projection (MVP) matrix in double
    double mvpMatrixD[16] = {0};
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            mvpMatrixD[i + j * 4] = 0.0;
            for (int k = 0; k < 4; k++)
            {
                mvpMatrixD[i + j * 4] += projectionMatrixD[i + k * 4] * modelViewMatrixD[k + j * 4];
            }
        }
    }

    // Convert to float only at the last moment
    GLfloat mvpMatrix[16];
    for (int i = 0; i < 16; ++i)
    {
        mvpMatrix[i] = static_cast<GLfloat>(mvpMatrixD[i]);
    }

    // Get uniform location for MVP
    GLint mvpLocation = glGetUniformLocation(cubeShader, "mvp");
    glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, mvpMatrix); // Set the MVP matrix uniform

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36); // Draw the cube
}

void glFrustum(float left, float right, float bottom, float top, float near, float far, GLfloat *projectionMatrix)
{
    std::memset(projectionMatrix, 0, 16 * sizeof(GLfloat)); // Initialize to zero

    projectionMatrix[0] = (2.0f * near) / (right - left);       // Set horizontal scaling
    projectionMatrix[5] = (2.0f * near) / (top - bottom);       // Set vertical scaling
    projectionMatrix[8] = (right + left) / (right - left);      // Set horizontal translation
    projectionMatrix[9] = (top + bottom) / (top - bottom);      // Set vertical translation
    projectionMatrix[10] = -(far + near) / (far - near);        // Set depth scaling
    projectionMatrix[11] = -1.0f;                               // Set perspective divide
    projectionMatrix[14] = -(2.0f * far * near) / (far - near); // Set depth translation
}

void Renderer::buildProjectionMatrix(const cv::Mat &cameraMatrix, int screen_w, int screen_h, GLfloat *projectionMatrix)
{
    float near = 0.1f;   // Near clipping plane
    float far = 3000.0f; // Far clipping plane

    // Extract focal lengths and principal point from camera matrix
    float fx = static_cast<float>(cameraMatrix.at<double>(0, 0)); // Focal length in x
    float fy = static_cast<float>(cameraMatrix.at<double>(1, 1)); // Focal length in y
    float cx = static_cast<float>(cameraMatrix.at<double>(0, 2)); // Principal point x
    float cy = static_cast<float>(cameraMatrix.at<double>(1, 2)); // Principal point y

    std::cout << "Camera Intrinsics: fx=" << fx << ", fy=" << fy << ", cx=" << cx << ", cy=" << cy << std::endl;
    std::cout << "Screen size: " << screen_w << "x" << screen_h << std::endl;

    // Calculate frustum boundaries
    float left = (0 - cx) / fx * near;          // Left boundary of the frustum
    float right = (screen_w - cx) / fx * near;  // Right boundary of the frustum
    float bottom = (cy - screen_h) / fy * near; // OpenCV's Y is top-down, OpenGL's is bottom-up, so invert
    float top = cy / fy * near;                 // Top boundary of the frustum

    // Build the projection matrix using the frustum values
    glFrustum(left, right, bottom, top, near, far, projectionMatrix);
}