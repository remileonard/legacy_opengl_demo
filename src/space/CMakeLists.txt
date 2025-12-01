# List all source files manually to avoid including X11 versions
set(SPACE_SOURCE
    events_glut.c
    main_glut.c
    image_compat.c
    fract.c
    god.c
    imlib.c
    input.c
    light.c
    matrix.c
    menu.c
    objects.c
    ringworld.c
    sols.c
    sound.c
    texture.c
    watch.c
    world.c
)

file (GLOB SPACE_HEADER "*.h")

if(WIN32)
    add_executable(space WIN32 ${SPACE_SOURCE} ${SPACE_HEADER})
    set_target_properties( space PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"" )
else()
    add_executable(space ${SPACE_SOURCE} ${SPACE_HEADER})
endif()
if (WIN32)
    target_link_libraries(space OpenGL::GL OpenGL::GLU GLUT::GLUT)
else()
    target_link_libraries(space OpenGL::GL OpenGL::GLU GLUT::GLUT m)
endif()
target_include_directories(space PRIVATE ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(space PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(space PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})