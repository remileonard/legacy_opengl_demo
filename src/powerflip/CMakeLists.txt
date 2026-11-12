cmake_minimum_required(VERSION 3.10)
project(powerflip C)

set(CMAKE_C_STANDARD 99)

# OpenGL et GLUT
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Configuration Windows
if(WIN32)
    add_definitions(-D_WIN32)
    add_definitions(-D_USE_MATH_DEFINES)  # Pour M_PI sur MSVC
    set(PLATFORM_LIBS ws2_32)
endif()

# Configuration Linux
if(UNIX)
    set(PLATFORM_LIBS m)
endif()

add_executable(powerflip
    powerflip.c
    pfobj.c
    yaodl.c
    light.c
)

target_include_directories(powerflip PRIVATE
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(powerflip
    OpenGL::GL
    OpenGL::GLU
    $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static>
    ${PLATFORM_LIBS}
    porting
    libdemo
)
