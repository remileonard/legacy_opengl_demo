project(libdemo C)

file (GLOB LIBDEMO_SOURCE "*.c")
file (GLOB LIBDEMO_HEADER "*.h")

add_library(libdemo ${LIBDEMO_SOURCE} ${LIBDEMO_HEADER})

if (WIN32)
    target_link_libraries(libdemo OpenGL::GL OpenGL::GLU $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static> porting)
else()
    target_link_libraries(libdemo OpenGL::GL OpenGL::GLU $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static> m porting)
endif()
target_include_directories(libdemo PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_options(libdemo PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(libdemo PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})