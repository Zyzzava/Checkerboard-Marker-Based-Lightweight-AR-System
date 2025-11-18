cmake_minimum_required(VERSION 3.16)
project(lightweight_ar VERSION 1.0 DESCRIPTION "Minimal marker-based AR starter" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(OpenCV REQUIRED COMPONENTS core highgui imgproc calib3d)

add_executable(lightweight_ar main.cpp calibrator.cpp)
target_link_libraries(lightweight_ar PRIVATE ${OpenCV_LIBS})
