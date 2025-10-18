file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(distort ${SC_SOURCE} ${SC_HEADER})
target_link_libraries(distort OpenGL::GL GLUT::GLUT)
target_include_directories(distort PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(distort PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(distort PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})