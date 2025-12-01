cmake_minimum_required(VERSION 3.26.0)
project("OpenGL Glut Legacy Demos" VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_subdirectory(src/porting)

add_subdirectory(src/libtk)
add_subdirectory(src/libgobj)
add_subdirectory(src/libdemo)


add_subdirectory(src/smooth)
add_subdirectory(src/ideas)
add_subdirectory(src/atlantis)
add_subdirectory(src/roller_coaster)
add_subdirectory(src/bounce)
add_subdirectory(src/distort)
add_subdirectory(src/gl_puzzle)
add_subdirectory(src/demos)
add_subdirectory(src/buttonfly)
add_subdirectory(src/insect)
add_subdirectory(src/cycles)
add_subdirectory(src/objviewer)
add_subdirectory(src/flip)
add_subdirectory(src/space)