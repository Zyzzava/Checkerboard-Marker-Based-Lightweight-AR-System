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
        -15.f, -15.f, -15.f, 1.f, 0.f, 0.f,
        15.f, -15.f, -15.f, 1.f, 0.f, 0.f,
        15.f, 15.f, -15.f, 1.f, 0.f, 0.f,
        15.f, 15.f, -15.f, 1.f, 0.f, 0.f,
        -15.f, 15.f, -15.f, 1.f, 0.f, 0.f,
        -15.f, -15.f, -15.f, 1.f, 0.f, 0.f,
        // all 36 vertices for a cube here with colors
        -15.f, -15.f, 15.f, 0.f, 1.f, 0.f,
        15.f, -15.f, 15.f, 0.f, 1.f, 0.f,
        15.f, 15.f, 15.f, 0.f, 1.f, 0.f,
        15.f, 15.f, 15.f, 0.f, 1.f, 0.f,
        -15.f, 15.f, 15.f, 0.f, 1.f, 0.f,
        -15.f, -15.f, 15.f, 0.f, 1.f, 0.f,

        -15.f, 15.f, 15.f, 0.f, 0.f, 1.f,
        -15.f, 15.f, -15.f, 0.f, 0.f, 1.f,
        -15.f, -15.f, -15.f, 0.f, 0.f, 1.f,
        -15.f, -15.f, -15.f, 0.f, 0.f, 1.f,
        -15.f, -15.f, 15.f, 0.f, 0.f, 1.f,
        -15.f, 15.f, 15.f, 0.f, 0.f, 1.f,

        15.f, 15.f, 15.f, 1.f, 1.f, 0.f,
        15.f, 15.f, -15.f, 1.f, 1.f, 0.f,
        15.f, -15.f, -15.f, 1.f, 1.f, 0.f,
        15.f, -15.f, -15.f, 1.f, 1.f, 0.f,
        15.f, -15.f, 15.f, 1.f, 1.f, 0.f,
        15.f, 15.f, 15.f, 1.f, 1.f, 0.f,

        -15.f, -15.f, -15.f, 1.f, 0.f, 1.f,
        15.f, -15.f, -15.f, 1.f, 0.f, 1.f,
        15.f, -15.f, 15.f, 1.f, 0.f, 1.f,
        15.f, -15.f, 15.f, 1.f, 0.f, 1.f,
        -15.f, -15.f, 15.f, 1.f, 0.f, 1.f,
        -15.f, -15.f, -15.f, 1.f, 0.f, 1.f,

        -15.f, 15.f, -15.f, 0.f, 1.f, 1.f,
        15.f, 15.f, -15.f, 0.f, 1.f, 1.f,
        15.f, 15.f, 15.f, 0.f, 1.f, 1.f,
        15.f, 15.f, 15.f, 0.f, 1.f, 1.f,
        -15.f, 15.f, 15.f, 0.f, 1.f, 1.f,
        -15.f, 15.f, -15.f, 0.f, 1.f, 1.f};
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
    glEnable(GL_DEPTH_TEST);
    glUseProgram(cubeShader);

    // Get uniform locations
    GLint modelViewLoc = glGetUniformLocation(cubeShader, "modelViewMatrix");
    GLint projLoc = glGetUniformLocation(cubeShader, "projectionMatrix");

    // Pass matrices to the shader
    glUniformMatrix4fv(modelViewLoc, 1, GL_FALSE, modelViewMatrix);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projectionMatrix);

    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void Renderer::buildProjectionMatrix(const cv::Mat &cameraMatrix, int screen_w, int screen_h, GLfloat *projectionMatrix)
{
    float near = 10.0f;  // Near clipping plane (e.g., 1cm)
    float far = 1000.0f; // Far clipping plane (e.g., 2m)

    // get camera matrix parameters
    float fx = static_cast<float>(cameraMatrix.at<double>(0, 0));
    float fy = static_cast<float>(cameraMatrix.at<double>(1, 1));
    float cx = static_cast<float>(cameraMatrix.at<double>(0, 2));
    float cy = static_cast<float>(cameraMatrix.at<double>(1, 2));

    // build projection matrix with zeros
    std::memset(projectionMatrix, 0, 16 * sizeof(GLfloat));

    // fill projection matrix with values
    projectionMatrix[0] = 2.0f * fx / screen_w;
    projectionMatrix[5] = 2.0f * fy / screen_h;

    projectionMatrix[8] = 2.0f * (cx / screen_w) - 1.0f;

    projectionMatrix[9] = 2.0f * (cy / screen_h) - 1.0f;
    projectionMatrix[10] = -(far + near) / (far - near);
    projectionMatrix[11] = -1.0f;
    projectionMatrix[14] = -2.0f * far * near / (far - near);
}

/*

// 😍
// LEGACY CODE
void initOpenGL(GLFWwindow **window, int width, int height, GLuint *cameraTexture)
{
    if (!glfwInit())
    {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    if (width <= 0 || height <= 0)
    {
        width = 1280;
        height = 720;
    }

    *window = glfwCreateWindow(width, height, "Augmented Reality", nullptr, nullptr);
    if (!*window)
    {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    glfwMakeContextCurrent(*window);
    glfwSwapInterval(1);

    if (glewInit() != GLEW_OK)
    {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(*window);
        glfwTerminate();
        *window = nullptr;
        return;
    }

    // create texture for camera frame
    glGenTextures(1, cameraTexture);
    glBindTexture(GL_TEXTURE_2D, *cameraTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
}

void displayScene(const cv::Mat &frame, GLuint texture)
{
    cv::Mat rgb;
    cv::cvtColor(frame, rgb, cv::COLOR_BGR2RGB);
    cv::flip(rgb, rgb, 0);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rgb.cols, rgb.rows, GL_RGB, GL_UNSIGNED_BYTE, rgb.data);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLE_STRIP);
    glTexCoord2f(0.f, 0.f);
    glVertex2f(-1.f, -1.f);
    glTexCoord2f(1.f, 0.f);
    glVertex2f(1.f, -1.f);
    glTexCoord2f(0.f, 1.f);
    glVertex2f(-1.f, 1.f);
    glTexCoord2f(1.f, 1.f);
    glVertex2f(1.f, 1.f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);

    glPopMatrix(); // modelview
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

void renderScene(const cv::Mat &rmat, const cv::Mat &tvec, const GLfloat *projectionMatrix)
{
    // Switch OpenGL into projection matrix mode so subsequent matrix loads affect the projection transform
    glMatrixMode(GL_PROJECTION);

    // Load the projection matrix
    glLoadMatrixf(projectionMatrix);

    // Switch to modelview mode for camera/scene transforms
    glMatrixMode(GL_MODELVIEW);

    // Pack the rotation matrix (from rmat) and translation vector (from tvec) into a 4x4 modelview matrix
    GLfloat modelview[16] = {
        static_cast<GLfloat>(rmat.at<double>(0, 0)), static_cast<GLfloat>(rmat.at<double>(1, 0)), static_cast<GLfloat>(rmat.at<double>(2, 0)), 0.0f,
        static_cast<GLfloat>(rmat.at<double>(0, 1)), static_cast<GLfloat>(rmat.at<double>(1, 1)), static_cast<GLfloat>(rmat.at<double>(2, 1)), 0.0f,
        static_cast<GLfloat>(rmat.at<double>(0, 2)), static_cast<GLfloat>(rmat.at<double>(1, 2)), static_cast<GLfloat>(rmat.at<double>(2, 2)), 0.0f,
        static_cast<GLfloat>(tvec.at<double>(0)), static_cast<GLfloat>(tvec.at<double>(1)), static_cast<GLfloat>(tvec.at<double>(2)), 1.0f};

    // Flip y, z axes to match OpenGL coordinate system
    glScalef(1.0f, -1.0f, -1.0f);

    // Load the modelview matrix
    glLoadMatrixf(modelview);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glBegin(GL_LINES);
    glColor3f(1, 0, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(30, 0, 0);
    glColor3f(0, 1, 0);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 30, 0);
    glColor3f(0, 0, 1);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, -30);
    glEnd();
    glfwSwapBuffers(glfwGetCurrentContext());
    glfwPollEvents();
}
*/