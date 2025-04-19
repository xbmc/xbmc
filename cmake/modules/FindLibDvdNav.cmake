#.rst:
# FindLibDvdNav
# ----------
# Finds the dvdnav library
#
# This will define the following target:
#
#   LibDvdNav::LibDvdNav   - The LibDvdNav library

if(NOT TARGET LibDvdNav::LibDvdNav)

  find_package(LibDvdRead MODULE REQUIRED ${SEARCH_QUIET})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libdvdnav)

  SETUP_BUILD_VARS()

  # Legacy support for lowercase user provided URL override
  if(libdvdnav_URL)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL ${libdvdnav_URL})
  endif()

  set(LIBDVDNAV_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

  set(HOST_ARCH ${ARCH})
  if(CORE_SYSTEM_NAME STREQUAL android)
    if(ARCH STREQUAL arm)
      set(HOST_ARCH arm-linux-androideabi)
    elseif(ARCH STREQUAL i486-linux)
      set(HOST_ARCH i686-linux-android)
    elseif()
      set(HOST_ARCH ${ARCH}-linux-android)
    endif()
  elseif(CORE_SYSTEM_NAME STREQUAL windowsstore)
    set(LIBDVD_ADDITIONAL_ARGS "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}" "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
  endif()

  string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS "-D_XBMC")

  if(APPLE)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LDFLAGS "-framework CoreFoundation")
    string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS " -D__DARWIN__")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LDFLAGS " -framework IOKit")
    endif()
  endif()

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS -DDUMMY_DEFINE=ON
                   ${LIBDVD_ADDITIONAL_ARGS})
  else()

    # INTERFACE_INCLUDE_DIRECTORIES may have multiple paths. We need to separate these
    # individually to then set the -I argument correctly with each path
    get_target_property(_interface_include_dirs LibDvdRead::LibDvdRead INTERFACE_INCLUDE_DIRECTORIES)
    foreach(_interface_include_dir ${_interface_include_dirs})
      string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS " -I${_interface_include_dir}")
    endforeach()

    if(TARGET LibDvdCSS::LibDvdCSS)
      string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS " -I$<TARGET_PROPERTY:LibDvdCSS::LibDvdCSS,INTERFACE_INCLUDE_DIRECTORIES> $<$<TARGET_EXISTS:LibDvdCSS::LibDvdCSS>:-D$<TARGET_PROPERTY:LibDvdCSS::LibDvdCSS,INTERFACE_COMPILE_DEFINITIONS>>")
    endif()

    find_program(AUTORECONF autoreconf REQUIRED)
    if (CMAKE_HOST_SYSTEM_NAME MATCHES "(Free|Net|Open)BSD")
      find_program(MAKE_EXECUTABLE gmake)
    endif()
    find_program(MAKE_EXECUTABLE make REQUIRED)

    set(CONFIGURE_COMMAND ${AUTORECONF} -vif
                  COMMAND ac_cv_path_GIT= ./configure
                          --target=${HOST_ARCH}
                          --host=${HOST_ARCH}
                          --enable-static
                          --disable-shared
                          --with-pic
                          --prefix=${DEPENDS_PATH}
                          --libdir=${DEPENDS_PATH}/lib
                          "CC=${CMAKE_C_COMPILER}"
                          "CFLAGS=${CMAKE_C_FLAGS} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS}"
                          "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LDFLAGS}"
                          "PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig")

    set(BUILD_COMMAND ${MAKE_EXECUTABLE})
    set(INSTALL_COMMAND ${MAKE_EXECUTABLE} install)
    set(BUILD_IN_SOURCE 1)
  endif()

  BUILD_DEP_TARGET()

  add_dependencies(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC} LibDvdRead::LibDvdRead)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LibDvdNav
                                    REQUIRED_VARS LIBDVDNAV_LIBRARY LIBDVDNAV_INCLUDE_DIR
                                    VERSION_VAR LIBDVDNAV_VERSION)

  if(LibDvdNav_FOUND)
    add_library(LibDvdNav::LibDvdNav UNKNOWN IMPORTED)
    set_target_properties(LibDvdNav::LibDvdNav PROPERTIES
                                               IMPORTED_LOCATION "${LIBDVDNAV_LIBRARY}"
                                               INTERFACE_LINK_LIBRARIES LibDvdRead::LibDvdRead
                                               INTERFACE_INCLUDE_DIRECTORIES "${LIBDVDNAV_INCLUDE_DIR}")

    add_dependencies(LibDvdNav::LibDvdNav libdvdnav)
  else()
    if(LibDvdNav_FIND_REQUIRED)
      message(FATAL_ERROR "Libdvdnav not found")
    endif()
  endif()
endif()
