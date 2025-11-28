cmake_minimum_required(VERSION 3.10)
project(flip C)

set(CMAKE_C_STANDARD 99)

# OpenGL et GLUT
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

# Threads
find_package(Threads REQUIRED)

# Configuration Windows
if(WIN32)
    add_definitions(-D_WIN32)
    add_definitions(-D_USE_MATH_DEFINES)  # Pour M_PI sur MSVC
    set(PLATFORM_LIBS ws2_32)
endif()

# Configuration Linux
if(UNIX)
    set(PLATFORM_LIBS m pthread)
endif()

# Sources
file(GLOB FLIP_SOURCES "*.c")

# Executable
if(WIN32)
    add_executable(flip
        ${FLIP_SOURCES}
    )
# set_target_properties( flip PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
else()
    add_executable(flip
        ${FLIP_SOURCES}
    )
endif()
target_include_directories(flip PRIVATE
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/flip
)

target_link_libraries(flip
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${PLATFORM_LIBS}
    porting
    libdemo
)