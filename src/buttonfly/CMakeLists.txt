file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(buttonfly WIN32 ${SC_SOURCE} ${SC_HEADER})
set_target_properties( buttonfly PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
target_link_libraries(buttonfly OpenGL::GL GLUT::GLUT)
target_include_directories(buttonfly PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(buttonfly PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(buttonfly PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})