# F-18 demo (Performer 2.2 and Coryphaeus EasyScene): the viewer of the
# Designer's Workbench files reuses the Performer layer of Mekton
set(PF_DIR ${CMAKE_SOURCE_DIR}/src/mekton)
set(F18_VIEW_SOURCES f18_view.c dwb.c dwb.h ${PF_DIR}/pf.c ${PF_DIR}/pfb.c)

if(NOT WIN32)
    add_executable(f18_view ${F18_VIEW_SOURCES})
else()
    add_executable(f18_view WIN32 ${F18_VIEW_SOURCES})
    set_target_properties(f18_view PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"")
endif()
if (WIN32)
    target_link_libraries(f18_view OpenGL::GL OpenGL::GLU $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static>)
else()
    target_link_libraries(f18_view $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static> OpenGL::GLU OpenGL::GL m)
endif()
target_include_directories(f18_view PRIVATE ${PF_DIR} ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
target_compile_options(f18_view PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(f18_view PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})
