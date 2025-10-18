file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(rc WIN32 ${SC_SOURCE} ${SC_HEADER})
set_target_properties( rc PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
target_link_libraries(rc OpenGL::GL GLUT::GLUT)
target_include_directories(rc PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(rc PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(rc PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})