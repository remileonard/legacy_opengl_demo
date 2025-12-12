cmake_minimum_required(VERSION 3.26.0)
project("OpenGL Glut Legacy Demos" VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)

add_subdirectory(src/porting)

add_subdirectory(src/libtk)
add_subdirectory(src/libgobj)
add_subdirectory(src/libdemo)

add_subdirectory(src/smooth)
add_subdirectory(src/ideas)
add_subdirectory(src/atlantis)
add_subdirectory(src/roller_coaster)
add_subdirectory(src/bounce)
add_subdirectory(src/distort)
add_subdirectory(src/gl_puzzle)
add_subdirectory(src/demos)
add_subdirectory(src/buttonfly)
add_subdirectory(src/insect)
add_subdirectory(src/cycles)
add_subdirectory(src/objviewer)
add_subdirectory(src/flip)
add_subdirectory(src/space)
add_subdirectory(src/flight)
add_subdirectory(src/iris)

set (BUTTON_FLY
    buttonfly
)
set(LEGACY_INTERACTIVE
    cycles
    distort
    enterprise
    flight
    flip
    gl_puzzle
    insect
    maze
    objviewer
    road
    smooth
    space
)
set (LEGACY_DEMOS
    bounce
    atlantis
    glutplane
    ideas
    scube
    sgi_bounce
    reflexion
    plasma
    rc
    stars
)

# Répertoire de distribution
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist")

set(DEMO_DIR "${DIST_DIR}/demos")
set(INTERACTIVE_DIR "${DIST_DIR}/interactive")

# Cible personnalisée pour créer le dossier de distribution
add_custom_target(dist ALL
    COMMENT "Création du dossier de distribution et copie des exécutables + data"
)

# Pour chaque exécutable, copie vers dist/
foreach(demo ${BUTTON_FLY})
    add_dependencies(dist ${demo})

    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIST_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${demo}>"
            "${DIST_DIR}/"
        COMMENT "Copie de l'exécutable ${demo} vers ${DIST_DIR}\n"
    )
endforeach()

foreach(demo ${LEGACY_DEMOS})
    add_dependencies(dist ${demo})

    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DEMO_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${demo}>"
            "${DEMO_DIR}/"
        COMMENT "Copie de l'exécutable ${demo} vers ${DEMO_DIR}\n"
    )
endforeach()

foreach(demo ${LEGACY_INTERACTIVE})
    add_dependencies(dist ${demo})

    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${INTERACTIVE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${demo}>"
            "${INTERACTIVE_DIR}/"
        COMMENT "Copie de l'exécutable ${demo} vers ${INTERACTIVE_DIR}\n"
    )
endforeach()

# Copie du dossier data (s'il existe à la racine du projet)
if(EXISTS "${CMAKE_SOURCE_DIR}/data")
    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIST_DIR}/data"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_SOURCE_DIR}/data"
            "${DIST_DIR}/data"
        COMMENT "Copie des données vers ${DIST_DIR}/data\n"
    )
endif()
if (EXISTS "${CMAKE_SOURCE_DIR}/data/buttonfly/menu")
    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
            "${CMAKE_SOURCE_DIR}/data/buttonfly/menu"
            "${DIST_DIR}/menu"
        COMMENT "Copie des menu pour buttonfly\n"
    )
endif()

file(GLOB_RECURSE ALL_DLLS
    "${CMAKE_BINARY_DIR}/*.dll"
)

foreach(dll ${ALL_DLLS})
    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy
            "${dll}"
            "${DIST_DIR}/"
        COMMAND "${CMAKE_COMMAND}" -E copy
            "${dll}"
            "${DEMO_DIR}/"
        COMMAND "${CMAKE_COMMAND}" -E copy
            "${dll}"
            "${INTERACTIVE_DIR}/"
        COMMENT "Copie de la DLL ${dll} vers ${DIST_DIR}\n"
    )
endforeach()