file (GLOB SC_SOURCE "*.c")
file (GLOB SC_HEADER "*.h")

add_executable(insect WIN32 ${SC_SOURCE} ${SC_HEADER})

set_target_properties( insect PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )

target_link_libraries(insect OpenGL::GL GLUT::GLUT libtk)
target_include_directories(insect PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_include_directories(insect PRIVATE ../libtk)
target_compile_options(insect PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(insect PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})
