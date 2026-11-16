# perfly, the database fly-through of IRIS Performer 2.0, and its common
# code: the original sources (perfly/ and common/) built unchanged on the
# Performer layer of src/porting/performer, with the scene graph and the
# .pfb loader of Mekton and the .dwb loader of the F-18 demo
set(PF_DIR ${CMAKE_SOURCE_DIR}/src/mekton)
set(PFC_DIR ${CMAKE_SOURCE_DIR}/src/porting/performer)
set(PERFLY_SOURCES
    perfly/perfly.c perfly/gui.c perfly/keybd.c perfly/cmdline.c
    common/main.c common/generic.c common/env.c paths/perfly_paths.c
    ${PFC_DIR}/pf_runtime.c ${PFC_DIR}/pf_scene.c ${PFC_DIR}/pfd_util.c ${PFC_DIR}/pfd_flt.c
    ${PFC_DIR}/pfu_util.c ${PFC_DIR}/pfu_gui.c ${PFC_DIR}/pfu_path.c ${PFC_DIR}/pfu_face.c ${PFC_DIR}/pfi_xformer.c
    ${PF_DIR}/pf.c ${PF_DIR}/pfb.c ${CMAKE_SOURCE_DIR}/src/f18/dwb.c)
# the original main becomes perfly_main: paths/perfly_paths.c takes the
# vehicles (-A model,path,name) out of the command line before it
set_source_files_properties(common/main.c PROPERTIES COMPILE_DEFINITIONS main=perfly_main)
if(WIN32)
    list(APPEND PERFLY_SOURCES ${PFC_DIR}/win32/getopt.c)
endif()

if(NOT WIN32)
    add_executable(perfly ${PERFLY_SOURCES})
else()
    add_executable(perfly WIN32 ${PERFLY_SOURCES})
    set_target_properties(perfly PROPERTIES LINK_FLAGS "/ENTRY:\"mainCRTStartup\"")
endif()
if (WIN32)
    target_link_libraries(perfly OpenGL::GL OpenGL::GLU $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static>)
    target_include_directories(perfly PRIVATE ${PFC_DIR}/win32)
    target_compile_definitions(perfly PRIVATE _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)
else()
    target_link_libraries(perfly $<IF:$<TARGET_EXISTS:FreeGLUT::freeglut>,FreeGLUT::freeglut,FreeGLUT::freeglut_static> OpenGL::GLU OpenGL::GL m)
endif()
# the headers of IRIX and Performer come before those of the system
target_include_directories(perfly BEFORE PRIVATE ${PFC_DIR}/include)
target_include_directories(perfly PRIVATE perfly common ${PFC_DIR} ${PF_DIR}
    ${CMAKE_SOURCE_DIR}/src/f18 ${OPENGL_INCLUDE_DIRS} ${GLUT_INCLUDE_DIRS})
# the 1996 code of SGI: old style declarations and pointer conversions
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(perfly PRIVATE -Wno-incompatible-pointer-types
        -Wno-implicit-int -Wno-int-conversion -Wno-implicit-function-declaration
        -Wno-pointer-sign -Wno-format-security)
endif()
target_compile_options(perfly PRIVATE ${OPENGL_CFLAGS} ${GLUT_CFLAGS})
target_link_options(perfly PRIVATE ${OPENGL_LDFLAGS} ${GLUT_LDFLAGS})
