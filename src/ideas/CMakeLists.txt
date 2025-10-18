file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(ideas WIN32 ${SC_SOURCE} ${SC_HEADER})
set_target_properties( ideas PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
target_link_libraries(ideas OpenGL::GL GLUT::GLUT)
target_include_directories(ideas PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(ideas PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(ideas PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})