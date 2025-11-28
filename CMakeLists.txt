cmake_minimum_required(VERSION 3.16)
project(lightweight_ar VERSION 1.0 DESCRIPTION "Minimal marker-based AR starter" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc calib3d)
find_package(nlohmann_json REQUIRED)
find_package(glfw3 3.3 REQUIRED)
find_package(GLEW REQUIRED)

add_executable(lightweight_ar main.cpp calibrator.cpp augmentor.cpp openGLrenderer.cpp jsonHelper.cpp statistics.cpp)

target_compile_definitions(lightweight_ar PRIVATE PROJECT_ROOT="${CMAKE_SOURCE_DIR}")

target_link_libraries(lightweight_ar PRIVATE nlohmann_json::nlohmann_json glfw GLEW::GLEW ${OpenCV_LIBS})