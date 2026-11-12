install(TARGETS ${BUTTON_FLY} RUNTIME DESTINATION .)
install(TARGETS ${LEGACY_DEMOS} RUNTIME DESTINATION demos)
install(TARGETS ${LEGACY_INTERACTIVE} RUNTIME DESTINATION interactive)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/" DESTINATION data)
install(FILES "${CMAKE_SOURCE_DIR}/data/buttonfly/menu" DESTINATION .)
install(FILES "${CMAKE_SOURCE_DIR}/packaging/windows/Start-Demos.cmd"
    "${CMAKE_SOURCE_DIR}/README.md" DESTINATION .)

# Resolve DLLs from targets at install time, not by globbing an unbuilt tree.
if(NOT CMAKE_MSVC_RUNTIME_LIBRARY OR CMAKE_MSVC_RUNTIME_LIBRARY MATCHES "DLL")
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
endif()
foreach(group BUTTON_FLY LEGACY_DEMOS LEGACY_INTERACTIVE)
    if(group STREQUAL "BUTTON_FLY")
        set(destination .)
    elseif(group STREQUAL "LEGACY_DEMOS")
        set(destination demos)
    else()
        set(destination interactive)
    endif()
    foreach(demo IN LISTS ${group})
        install(FILES $<TARGET_RUNTIME_DLLS:${demo}> DESTINATION "${destination}")
    endforeach()
    if(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        install(PROGRAMS ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS}
            DESTINATION "${destination}")
    endif()
endforeach()

if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
    install(FILES
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/share/freeglut/copyright"
        DESTINATION licenses/freeglut)
endif()

set(WINDOWS_PACKAGE_EXECUTABLES buttonfly.exe)
foreach(demo IN LISTS LEGACY_DEMOS)
    list(APPEND WINDOWS_PACKAGE_EXECUTABLES "demos/${demo}.exe")
endforeach()
foreach(demo IN LISTS LEGACY_INTERACTIVE)
    list(APPEND WINDOWS_PACKAGE_EXECUTABLES "interactive/${demo}.exe")
endforeach()
configure_file("${CMAKE_SOURCE_DIR}/cmake/VerifyWindowsPackage.cmake.in"
    "${CMAKE_BINARY_DIR}/VerifyWindowsPackage.cmake" @ONLY)

set(CPACK_GENERATOR ZIP)
set(CPACK_PACKAGE_NAME legacy-opengl-demo)
if(NOT DEFINED CPACK_PACKAGE_VERSION)
    set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
endif()
set(CPACK_PACKAGE_FILE_NAME
    "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-windows-x64")
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
include(CPack)
