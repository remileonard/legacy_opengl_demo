file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(distort WIN32 ${SC_SOURCE} ${SC_HEADER})
set_target_properties( distort PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
target_link_libraries(distort OpenGL::GL GLUT::GLUT)
target_include_directories(distort PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(distort PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(distort PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})