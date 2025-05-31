# FindEffects11
# -------
# Finds the Effects11 library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Effects11   - The Effects11 library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroEffects11)

    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-win-debugpostfix.patch")
    generate_patchcommand("${patches}")

    # Effects 11 cant be built using /permissive-
    # strip and manually set the rest of the build flags
    string(REPLACE "/permissive-" "" ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS ${CMAKE_CXX_FLAGS} )

    set(CMAKE_ARGS 
    "-DCMAKE_CXX_FLAGS=${EFFECTS_CXX_FLAGS} $<$<CONFIG:Debug>:${CMAKE_CXX_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_CXX_FLAGS_RELEASE}>"
    "-DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} $<$<CONFIG:Debug>:${CMAKE_EXE_LINKER_FLAGS_DEBUG}> $<$<CONFIG:Release>:${CMAKE_EXE_LINKER_FLAGS_RELEASE}>")

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_DEBUG_POSTFIX d)
    set(WIN_DISABLE_PROJECT_FLAGS ON)

    BUILD_DEP_TARGET()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

    # Make INCLUDE_DIR match cmake config output
    string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR "/Effects11")
    file(MAKE_DIRECTORY ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC effects11)

  SETUP_BUILD_VARS()

  SEARCH_EXISTING_PACKAGES()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    
    if(TARGET Microsoft::Effects11 AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS Microsoft::Effects11)
    else()
      SETUP_BUILD_TARGET()

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Effects11_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find or build Effects11 library.")
    endif()
  endif()
endif()
