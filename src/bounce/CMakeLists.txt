file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(sgi_bounce WIN32 ${SC_SOURCE} ${SC_HEADER})
set_target_properties( sgi_bounce PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
target_link_libraries(sgi_bounce OpenGL::GL GLUT::GLUT)
target_include_directories(sgi_bounce PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(sgi_bounce PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(sgi_bounce PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})