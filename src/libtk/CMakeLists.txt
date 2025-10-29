file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_library(libtk ${SC_SOURCE} ${SC_HEADER})


target_link_libraries(libtk OpenGL::GL GLUT::GLUT)
target_include_directories(libtk PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(libtk PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(libtk PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})