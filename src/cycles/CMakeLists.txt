cmake_minimum_required(VERSION 3.10)
project(cycles C)

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

# Désactiver l'audio pour l'instant (à implémenter plus tard)
add_definitions(-Uno_AUDIO)

# Sources
file(GLOB CYCLES_SOURCES "src/cycles/*.c")
file(GLOB PORTING_SOURCES "src/porting/*.c")

# Exclure sound.c si AUDIO n'est pas défini
list(FILTER CYCLES_SOURCES EXCLUDE REGEX ".*sound\\.c$")

# Executable
add_executable(cycles
    src/main.c
    ${CYCLES_SOURCES}
    ${PORTING_SOURCES}
)

target_include_directories(cycles PRIVATE
    ${OPENGL_INCLUDE_DIRS}
    ${GLUT_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/porting
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cycles
)

target_link_libraries(cycles
    ${OPENGL_LIBRARIES}
    ${GLUT_LIBRARIES}
    ${CMAKE_THREAD_LIBS_INIT}
    ${PLATFORM_LIBS}
)