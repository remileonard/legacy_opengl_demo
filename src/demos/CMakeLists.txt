
set(SIMPLE_EXECUTABLES
    bounce
    glutplane
    scube
    stars
    maze
)

foreach(target ${SIMPLE_EXECUTABLES})
    if(EXISTS "${CMAKE_SOURCE_DIR}/src/demos/${target}.c")
        if(NOT WIN32)
            add_executable(${target}  ${target}.c)
        else()
            add_executable(${target} WIN32  ${target}.c)
            set_target_properties( ${target} PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
        endif()
        if (WIN32)
            target_link_libraries(${target} OpenGL::GL OpenGL::GLU GLUT::GLUT)
        else()
            target_link_libraries(${target} OpenGL::GL OpenGL::GLU GLUT::GLUT m)
        endif()
        target_include_directories(${target} PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
        target_compile_options(${target} PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
        target_link_options(${target} PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})
    endif()
endforeach()