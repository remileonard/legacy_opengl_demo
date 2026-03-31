cmake_minimum_required(VERSION 3.26.0)
project("OpenGL Glut Legacy Demos" VERSION 1.0.0 LANGUAGES C CXX)
set(CMAKE_C_STANDARD 90)
set(CMAKE_C_STANDARD_REQUIRED ON)



if(APPLE)
    set(CMAKE_FIND_FRAMEWORK FIRST)
    include_directories(BEFORE ${CMAKE_SOURCE_DIR}/src/porting)
    add_compile_definitions(GL_SILENCE_DEPRECATION)
    add_compile_definitions(HZ=100)
    add_compile_options(
        -Wno-error=implicit-function-declaration
        -Wno-error=return-mismatch
        -Wno-error=int-conversion
        -Wno-error=incompatible-function-pointer-types
        -Wno-error=implicit-int
    )
endif()
find_package(OpenGL REQUIRED)
find_package(FreeGLUT CONFIG REQUIRED)


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
    cubetris
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
    particles
    mandel
    primes
    heat
    brick
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
if (WIN32)
    if (EXISTS "${CMAKE_SOURCE_DIR}/data/buttonfly/menu")
        add_custom_command(TARGET dist POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_SOURCE_DIR}/data/buttonfly/menu"
                "${DIST_DIR}/menu"
            COMMENT "Copie des menu pour buttonfly\n"
        )
    endif()
else()
    if (EXISTS "${CMAKE_SOURCE_DIR}/data/buttonfly_linux/menu")
        add_custom_command(TARGET dist POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy
                "${CMAKE_SOURCE_DIR}/data/buttonfly_linux/menu"
                "${DIST_DIR}/menu"
            COMMENT "Copie des menu pour buttonfly\n"
        )
    endif()
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

if(APPLE)
    set(APP_NAME "Flight")
    set(APP_BUNDLE_DIR "${CMAKE_BINARY_DIR}/${APP_NAME}.app")
    set(APP_CONTENTS_DIR "${APP_BUNDLE_DIR}/Contents")
    set(APP_MACOS_DIR "${APP_CONTENTS_DIR}/MacOS")
    set(APP_RESOURCES_DIR "${APP_CONTENTS_DIR}/Resources")

    add_custom_target(flight-app
        DEPENDS flight
        COMMENT "Creating Flight.app bundle and DMG"
    )

    add_custom_command(TARGET flight-app POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${APP_MACOS_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${APP_RESOURCES_DIR}/data"
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:flight>" "${APP_MACOS_DIR}/flight"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/data" "${APP_RESOURCES_DIR}/data"
        COMMAND bash "${CMAKE_SOURCE_DIR}/create_symlinks.sh" "${APP_RESOURCES_DIR}/data"
        COMMENT "Copying flight executable and data to app bundle"
    )

    file(WRITE "${CMAKE_BINARY_DIR}/Info.plist.in" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>flight</string>
    <key>CFBundleIdentifier</key>
    <string>com.sgi.flight</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>Flight</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.14</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>")

    add_custom_command(TARGET flight-app POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_BINARY_DIR}/Info.plist.in" "${APP_CONTENTS_DIR}/Info.plist"
        COMMAND hdiutil create -volname "Flight" -srcfolder "${APP_BUNDLE_DIR}" -ov -format UDZO "${CMAKE_BINARY_DIR}/Flight.dmg"
        COMMENT "Creating Info.plist and Flight.dmg"
    )

    set(SGIDEMOS_APP_NAME "SGIDemos")
    set(SGIDEMOS_APP_BUNDLE_DIR "${CMAKE_BINARY_DIR}/${SGIDEMOS_APP_NAME}.app")
    set(SGIDEMOS_APP_CONTENTS_DIR "${SGIDEMOS_APP_BUNDLE_DIR}/Contents")
    set(SGIDEMOS_APP_MACOS_DIR "${SGIDEMOS_APP_CONTENTS_DIR}/MacOS")
    set(SGIDEMOS_APP_RESOURCES_DIR "${SGIDEMOS_APP_CONTENTS_DIR}/Resources")

    add_custom_target(sgidemos-app
        DEPENDS buttonfly ${LEGACY_DEMOS} ${LEGACY_INTERACTIVE}
        COMMENT "Creating SGIDemos.app bundle and DMG"
    )

    add_custom_command(TARGET sgidemos-app POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SGIDEMOS_APP_MACOS_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SGIDEMOS_APP_MACOS_DIR}/demos"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SGIDEMOS_APP_MACOS_DIR}/interactive"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SGIDEMOS_APP_RESOURCES_DIR}/data"
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:buttonfly>" "${SGIDEMOS_APP_MACOS_DIR}/buttonfly-bin"
        COMMENT "Creating app bundle directories and copying buttonfly"
    )

    foreach(demo ${LEGACY_DEMOS})
        add_custom_command(TARGET sgidemos-app POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${demo}>" "${SGIDEMOS_APP_MACOS_DIR}/demos/${demo}"
            COMMENT "Copying ${demo} to demos directory"
        )
    endforeach()

    foreach(demo ${LEGACY_INTERACTIVE})
        add_custom_command(TARGET sgidemos-app POST_BUILD
            COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${demo}>" "${SGIDEMOS_APP_MACOS_DIR}/interactive/${demo}"
            COMMENT "Copying ${demo} to interactive directory"
        )
    endforeach()

    add_custom_command(TARGET sgidemos-app POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/data" "${SGIDEMOS_APP_RESOURCES_DIR}/data"
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/data/buttonfly_linux/menu" "${SGIDEMOS_APP_MACOS_DIR}/menu"
        COMMAND bash "${CMAKE_SOURCE_DIR}/create_symlinks.sh" "${SGIDEMOS_APP_RESOURCES_DIR}/data"
        COMMAND "${CMAKE_COMMAND}" -E create_symlink "../Resources/data" "${SGIDEMOS_APP_MACOS_DIR}/data"
        COMMENT "Copying data and menu to app bundle"
    )

    file(WRITE "${CMAKE_BINARY_DIR}/buttonfly-launcher.sh" "#!/bin/bash
SCRIPT_DIR=\"\$(cd \"\$(dirname \"\$0\")\" && pwd)\"
cd \"\$SCRIPT_DIR\"
exec \"\$SCRIPT_DIR/buttonfly-bin\" \"\$SCRIPT_DIR/menu\"
")

    add_custom_command(TARGET sgidemos-app POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_BINARY_DIR}/buttonfly-launcher.sh" "${SGIDEMOS_APP_MACOS_DIR}/buttonfly"
        COMMAND chmod +x "${SGIDEMOS_APP_MACOS_DIR}/buttonfly"
        COMMENT "Creating buttonfly launcher script"
    )

    file(WRITE "${CMAKE_BINARY_DIR}/SGIDemos.plist.in" "<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">
<plist version=\"1.0\">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleExecutable</key>
    <string>buttonfly</string>
    <key>CFBundleIdentifier</key>
    <string>com.sgi.demos</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>SGI Demos</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>1.0</string>
    <key>CFBundleVersion</key>
    <string>1</string>
    <key>LSMinimumSystemVersion</key>
    <string>10.14</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>")

    add_custom_command(TARGET sgidemos-app POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_BINARY_DIR}/SGIDemos.plist.in" "${SGIDEMOS_APP_CONTENTS_DIR}/Info.plist"
        COMMAND hdiutil create -volname "SGI Demons" -srcfolder "${SGIDEMOS_APP_BUNDLE_DIR}" -ov -format UDZO "${CMAKE_BINARY_DIR}/SGIDemos.dmg"
        COMMENT "Creating Info.plist and SGIDemos.dmg"
    )
endif()