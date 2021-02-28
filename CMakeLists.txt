CMAKE_MINIMUM_REQUIRED(VERSION 3.12)
PROJECT(assrender)

include(GNUInstallDirs)

ADD_SUBDIRECTORY(src)

IF(MINGW)
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -Wl,--add-stdcall-alias")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wpedantic")
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")
    SET(CMAKE_C_FLAGS_RELEASE "-O3")
ENDIF()

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
