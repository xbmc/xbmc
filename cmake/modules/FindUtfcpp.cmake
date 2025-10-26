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

  macro(buildmacroUtfcpp)
    # Install headeronly lib
    set(CMAKE_ARGS -DUMMYARGS=ON)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INTERFACE_LIB TRUE)
    set(BUILD_BYPRODUCTS ${DEPENDS_PATH}/include/utf8cpp/utf8.h)

    BUILD_DEP_TARGET()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR ${DEPENDS_PATH}/include/utf8cpp)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC utfcpp)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME utf8cpp)

  SETUP_BUILD_VARS()

  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/share/utf8cpp/cmake
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # extract INTERFACE_INCLUDE_DIRECTORIES for find_package_handle_standard_args usage
    get_target_property(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR utf8cpp::utf8cpp INTERFACE_INCLUDE_DIRECTORIES)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION})
  else()
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(Utfcpp
                                    REQUIRED_VARS ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR
                                    VERSION_VAR ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VERSION)

  if(Utfcpp_FOUND)
    if(TARGET utf8cpp::utf8cpp AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS utf8cpp::utf8cpp)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME)
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)
      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Utfcpp_FIND_REQUIRED)
      message(FATAL_ERROR "Utf8-cpp library was not found.")
    endif()
  endif()
endif()
