file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(smooth WIN32 ${SC_SOURCE} ${SC_HEADER})
set_target_properties( smooth PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
target_link_libraries(smooth OpenGL::GL GLUT::GLUT)
target_include_directories(smooth PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(smooth PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(smooth PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})