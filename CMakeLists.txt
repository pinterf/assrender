cmake_minimum_required(VERSION 3.15)
project(assrender)

include(GNUInstallDirs)

if(MINGW)
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -Wl,--add-stdcall-alias")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wpedantic")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "-s")
endif()

add_subdirectory(src)

# uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)

add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
