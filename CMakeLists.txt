cmake_minimum_required(VERSION 3.10)
project(mrk)

# set c++ standard to c++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set the build type to release
set(CMAKE_BUILD_TYPE Release)

# Include source folder
include_directories("${PROJECT_SOURCE_DIR}/source")

# Add the core executable
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/source/*.cpp")

add_executable(${PROJECT_NAME} ${SOURCES})

# Conditionally link pthread library for Linux
if (UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)
    target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
endif()