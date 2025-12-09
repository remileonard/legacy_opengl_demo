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

set(LEGACY_DEMOS
    atlantis
    bounce
    buttonfly
    cycles
    distort
    enterprise
    flight
    flip
    gl_puzzle
    glutplane
    ideas
    insect
    maze
    objviewer
    plasma
    rc
    reflexion
    road
    scube
    sgi_bounce
    smooth
    space
    stars
)

# Répertoire de distribution
set(DIST_DIR "${CMAKE_BINARY_DIR}/dist")

# Cible personnalisée pour créer le dossier de distribution
add_custom_target(dist ALL
    COMMENT "Création du dossier de distribution et copie des exécutables + data"
)

# Pour chaque exécutable, copie vers dist/
foreach(demo ${LEGACY_DEMOS})
    add_dependencies(dist ${demo})

    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIST_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:${demo}>"
            "${DIST_DIR}/"
        COMMENT "Copie de l'exécutable ${demo} vers ${DIST_DIR}"
    )
endforeach()

# Copie du dossier data (s'il existe à la racine du projet)
if(EXISTS "${CMAKE_SOURCE_DIR}/data")
    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${DIST_DIR}/data"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory
            "${CMAKE_SOURCE_DIR}/data"
            "${DIST_DIR}/data"
        COMMENT "Copie des données vers ${DIST_DIR}/data"
    )
endif()

file(GLOB_RECURSE ALL_DLLS
    "${CMAKE_BINARY_DIR}/*.dll"
)

foreach(dll ${ALL_DLLS})
    add_custom_command(TARGET dist POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${dll}"
            "${DIST_DIR}/"
        COMMENT "Copie de la DLL ${dll} vers ${DIST_DIR}"
    )
endforeach()