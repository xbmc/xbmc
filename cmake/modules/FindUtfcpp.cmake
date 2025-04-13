#.rst:
# FindUtfcpp
# ----------
# Finds the Utfcpp Header-only library
#
# This will define the following target ALIAS:
#
#   utf8cpp::utf8cpp   - The utf8-cpp header-only library
#   utf8::cpp          - ALIAS target to utf8cpp::utf8cpp
#   LIBRARY::Utfcpp    - ALIAS target to utf8cpp::utf8cpp
#

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  macro(buildutfcpp)
    # Install headeronly lib
    set(CMAKE_ARGS -DUMMYARGS=ON)

    set(BUILD_BYPRODUCTS ${DEPENDS_PATH}/include/utf8cpp/utf8.h)

    BUILD_DEP_TARGET()

    set(UTFCPP_INCLUDE_DIR ${DEPENDS_PATH}/include/utf8cpp)
    set(utf8cpp_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC utfcpp)

  SETUP_BUILD_VARS()

  find_package(utf8cpp CONFIG ${SEARCH_QUIET}
                       HINTS ${DEPENDS_PATH}/share/utf8cpp/cmake
                       ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  if(utf8cpp_FOUND)
    # extract INTERFACE_INCLUDE_DIRECTORIES for find_package_handle_standard_args usage
    get_target_property(UTFCPP_INCLUDE_DIR utf8cpp::utf8cpp INTERFACE_INCLUDE_DIRECTORIES)
  else()
    buildutfcpp()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Utfcpp
                                    REQUIRED_VARS UTFCPP_INCLUDE_DIR
                                    VERSION_VAR utf8cpp_VERSION)

  if(Utfcpp_FOUND)
    if(TARGET utf8cpp::utf8cpp AND NOT TARGET utfcpp)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS utf8cpp::utf8cpp)
    elseif(TARGET PkgConfig::utf8cpp AND NOT TARGET utfcpp)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::utf8cpp)
    else()
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                INTERFACE_INCLUDE_DIRECTORIES "${UTFCPP_INCLUDE_DIR}")
    endif()

    if(TARGET utfcpp)
      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} utfcpp)

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET utfcpp)
        buildutfcpp()
        set_target_properties(utfcpp PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends utfcpp)
    endif()
  else()
    if(Utfcpp_FIND_REQUIRED)
      message(FATAL_ERROR "Utf8-cpp library was not found.")
    endif()
  endif()
endif()
