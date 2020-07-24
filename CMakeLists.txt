cmake_minimum_required(VERSION 3.1)

set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_DISABLE_SOURCE_CHANGES  ON)

# Use std=c++14
set(CMAKE_CXX_STANDARD 14)

set (GCC_COVERAGE_COMPILE_FLAGS "-Wall -Wextra -Wno-unused-parameter")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}")

# Enable sanitizers for debug builds
if (CMAKE_BUILD_TYPE MATCHES Debug)
    # Enable extra debug info in binary
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-4 -g3 -Og")
else()
    set (GCC_OPTIMIZATION "-O9")
endif()

# Start our project
project(lissajous CXX)

# Need OpenGL for rendering
include(FindOpenGL)
include_directories(${OPENGL_INCLUDE_DIRS})

# And glfw for windowing
find_package(glfw3 REQUIRED)

add_executable(lissajous
    src/main.cpp
)
target_link_libraries(lissajous
    sndfile
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    glfw
)
