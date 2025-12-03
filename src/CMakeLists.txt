set(PluginName "assrender")

if (NOT WIN32)
  string(TOLOWER "${PluginName}" PluginName)
endif()

set(ProjectName "${PluginName}")
project(${ProjectName} LANGUAGES C)

file(GLOB ASSRender_SRC *.c)

if(WIN32)
  list(APPEND ASSRender_SRC "ASSRender.rc")
  if(NOT MINGW AND CMAKE_SIZEOF_VOID_P EQUAL 4)
      # 32 bit VS only: decoration remove for old Avisynth 2.6
      LIST(APPEND ASSRender_SRC "assrender.def")
  endif()
endif()

add_library(${PluginName} SHARED ${ASSRender_SRC})

set_target_properties(${PluginName} PROPERTIES "OUTPUT_NAME" "${PluginName}")
if (MINGW)
  set_target_properties(${PluginName} PROPERTIES PREFIX "")
  set_target_properties(${PluginName} PROPERTIES IMPORT_PREFIX "")
endif()

find_package(PkgConfig REQUIRED)

if (NOT WIN32 OR MINGW)
  if (NOT MINGW)
    PKG_CHECK_MODULES(AVISYNTH REQUIRED avisynth>=3.7.5)
  endif()
  target_include_directories(${PluginName} PRIVATE ${AVISYNTH_INCLUDE_DIR})
endif()

#dedicated include dir for avisynth.h
target_include_directories(${ProjectName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

PKG_CHECK_MODULES(LIBASS REQUIRED libass>=0.17.0)
target_include_directories(${PluginName} PRIVATE ${LIBASS_INCLUDE_DIRS})
target_link_libraries(${ProjectName} ${LIBASS_LINK_LIBRARIES})

if (WIN32)
  # avisynth.lib for C API
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(AVS_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/x86-64)
  else()
    set(AVS_LIBDIR ${CMAKE_CURRENT_SOURCE_DIR}/../lib/x86-32)
  endif()
  if (MINGW)
    target_link_libraries(${ProjectName} ${LIBASS_LDFLAGS} ${AVS_LIBDIR}/avisynth.lib)
  else()
    target_link_libraries(${ProjectName} ${AVS_LIBDIR}/avisynth.lib)
  endif()
else()
  target_link_libraries(${ProjectName} ${LIBASS_LDFLAGS} ${AVISYNTH_LDFLAGS})
endif()

include(GNUInstallDirs)
install(TARGETS ${ProjectName} LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
