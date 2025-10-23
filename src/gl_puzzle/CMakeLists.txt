file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(gl_puzzle WIN32 ${SC_SOURCE} ${SC_HEADER})
set_target_properties( gl_puzzle PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
target_link_libraries(gl_puzzle OpenGL::GL GLUT::GLUT)
target_include_directories(gl_puzzle PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(gl_puzzle PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(gl_puzzle PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})