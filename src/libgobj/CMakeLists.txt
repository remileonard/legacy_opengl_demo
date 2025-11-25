file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")



add_library(libgobj ${SC_SOURCE} ${SC_HEADER})


if (WIN32)
    target_link_libraries(libgobj OpenGL::GL OpenGL::GLU GLUT::GLUT porting)
else()
    target_link_libraries(libgobj OpenGL::GL OpenGL::GLU GLUT::GLUT m porting)
endif()
target_include_directories(libgobj PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS}  ${CMAKE_CURRENT_SOURCE_DIR}/..)

target_compile_options(libgobj PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(libgobj PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})