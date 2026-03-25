project(porting C)

file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_library(porting ${SC_SOURCE} ${SC_HEADER})

if (WIN32)
    target_link_libraries(porting OpenGL::GL OpenGL::GLU $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static> ws2_32)
else()
    target_link_libraries(porting OpenGL::GL OpenGL::GLU $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static> m)
endif()
target_include_directories(porting PUBLIC ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(porting PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(porting PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})