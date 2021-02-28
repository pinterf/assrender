set(PluginName "assrender")

if (NOT WIN32)
  string(TOLOWER "${PluginName}" PluginName)
endif()

set(ProjectName "${PluginName}")
project(${ProjectName} LANGUAGES C)

SET(ASSRender_SRC 
    assrender.c
    assrender.h
    render.c
    render.h
    sub.c
    sub.h
    timecodes.c
    timecodes.h
    )

IF( WIN32 )
    LIST(APPEND ASSRender_SRC "ASSRender.rc")
    if(NOT MINGW AND CMAKE_SIZEOF_VOID_P EQUAL 4)
        # 32 bit VS only: decoration remove for old Avisynth 2.6
        LIST(APPEND ASSRender_SRC "assrender.def")
    ENDIF()
ENDIF()

add_library(${PluginName} SHARED ${ASSRender_SRC})

set_target_properties(${PluginName} PROPERTIES "OUTPUT_NAME" "${PluginName}")
if (MINGW)
  set_target_properties(${PluginName} PROPERTIES PREFIX "")
  set_target_properties(${PluginName} PROPERTIES IMPORT_PREFIX "")
endif()

IF (NOT WIN32 OR MINGW)
  FIND_PACKAGE(PkgConfig REQUIRED)
  PKG_CHECK_MODULES(LIBASS REQUIRED libass>=0.12.0)
  target_include_directories(${PluginName} PRIVATE ${LIBASS_INCLUDE_DIR})
ENDIF()

#dedicated include dir for avisynth.h
target_include_directories(${ProjectName} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if (WIN32)
  # avisynth.lib for C API
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(AVS_LIBDIR  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/x86-64)
  else()
    set(AVS_LIBDIR  ${CMAKE_CURRENT_SOURCE_DIR}/../lib/x86-32)
  endif()
  if (MINGW)
    TARGET_LINK_LIBRARIES(${ProjectName} ${LIBASS_LDFLAGS} ${AVS_LIBDIR}/avisynth.lib)
  else()
    # msvc: IDE based, with subprojects
    TARGET_LINK_LIBRARIES(${ProjectName} ${AVS_LIBDIR}/avisynth.lib)
  endif()
else()
  TARGET_LINK_LIBRARIES(${ProjectName} ${LIBASS_LDFLAGS} )
endif()

include(GNUInstallDirs)

INSTALL(TARGETS ${ProjectName}
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/avisynth")
