#.rst:
# FindBOOST
# -------
# Finds the Boost libraries, optionally building them internally
#
# This will define the following variables::
#
#   BOOST_FOUND - system has boost
#   BOOST_INCLUDE_DIRS - the boost include directory
#   BOOST_LIBRARIES - the boost libraries
#
# and the following imported targets::
#
#   boost::boost - The boost library
#

# The list of boost libraries to find
list(APPEND BOOST_COMPONENTS "container")
list(APPEND BOOST_COMPONENTS "json")
list(APPEND BOOST_COMPONENTS "system")

# If target exists, no need to rerun find. Allows a module that may be a
# dependency for multiple libraries to just be executed once to populate all
# required variables/targets.
if(NOT TARGET boost::boost)
  if(ENABLE_INTERNAL_BOOST)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC boost)

    SETUP_BUILD_VARS()

    set(BOOST_VERSION ${${MODULE}_VER})

    set(CMAKE_ARGS "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}"
                   "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
                   "-DCMAKE_CXX_COMPILER_LAUNCHER=${CMAKE_CXX_COMPILER_LAUNCHER}"
                   "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
                   "-DCMAKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}"
                   "-DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}"
                   "-DCMAKE_STATIC_LINKER_FLAGS=${CMAKE_STATIC_LINKER_FLAGS}"
                   "-DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")

    set(PATCH_COMMAND
      ${CMAKE_COMMAND} -E copy
        "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/CMakeLists.txt"
        "${CMAKE_CURRENT_BINARY_DIR}/${CORE_BUILD_DIR}/${MODULE_LC}/src/${MODULE_LC}/CMakeLists.txt" &&
      ${CMAKE_COMMAND} -E copy
        "${CORE_SOURCE_DIR}/tools/depends/target/${MODULE_LC}/rename_boost_libraries.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/${CORE_BUILD_DIR}/${MODULE_LC}/src/${MODULE_LC}/rename_boost_libraries.cmake")

    # Ninja always needs a byproduct
    set(BUILD_BYPRODUCTS "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include/boost/system.hpp")

    BUILD_DEP_TARGET()

    foreach(BOOST_COMPONENT ${BOOST_COMPONENTS})
      list(APPEND BOOST_LIBRARIES "${DEPENDS_PATH}/lib/libboost_${BOOST_COMPONENT}${CMAKE_STATIC_LIBRARY_SUFFIX}")
    endforeach()
  else()
    # TODO
    #find_package(Boost REQUIRED COMPONENTS ${BOOST_COMPONENTS})

    #find_path(BOOST_INCLUDE_DIR NAMES boost/system.hpp)
    #find_library(BOOST_CONTAINER_LIBRARY NAMES boost_contianer)
    #find_library(BOOST_JSON_LIBRARY NAMES boost_json)
    #find_library(BOOST_SYSTEM_LIBRARY NAMES boost_system)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(BOOST
    REQUIRED_VARS
      BOOST_INCLUDE_DIR
      BOOST_LIBRARIES
    VERSION_VAR
      BOOST_VERSION
  )

  if(BOOST_FOUND)
    set(BOOST_INCLUDE_DIRS "${BOOST_INCLUDE_DIR}")

    add_library(boost::boost INTERFACE IMPORTED)

    set_target_properties(boost::boost PROPERTIES
      FOLDER "External Projects"
      # TODO
      #INTERFACE_COMPILE_DEFINITIONS "BOOST_SYMBOL_EXPORT" "BOOST_SYMBOL_IMPORT" "BOOST_USE_WINDOWS_H"
      INTERFACE_INCLUDE_DIRECTORIES "${BOOST_INCLUDE_DIR}")

    if(TARGET boost)
      add_dependencies(boost::boost boost)
    endif()

    # Add dependency to libkodi to build
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP boost::boost)
  else()
    if(BOOST_FIND_REQUIRED)
      message(FATAL_ERROR "boost not found. Maybe use -DENABLE_INTERNAL_BOOST=ON")
    endif()
  endif()

  mark_as_advanced(BOOST_INCLUDE_DIR)
endif()
