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
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "-framework CoreFoundation")
    string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS " -D__DARWIN__")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "-framework IOKit")
    endif()
    string(REPLACE ";" " " ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LDFLAGS "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES}")
  endif()

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS -DDUMMY_DEFINE=ON
                   -DCMAKE_POLICY_VERSION_MINIMUM=3.5
                   ${LIBDVD_ADDITIONAL_ARGS})
  else()

    # INTERFACE_INCLUDE_DIRECTORIES may have multiple paths. We need to separate these
    # individually to then set the -I argument correctly with each path
    get_target_property(_interface_include_dirs ${APP_NAME_LC}::LibDvdRead INTERFACE_INCLUDE_DIRECTORIES)
    foreach(_interface_include_dir ${_interface_include_dirs})
      string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS " -I${_interface_include_dir}")
    endforeach()

    if(TARGET ${APP_NAME_LC}::LibDvdCSS)
      string(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CFLAGS " -I$<TARGET_PROPERTY:${APP_NAME_LC}::LibDvdCSS,INTERFACE_INCLUDE_DIRECTORIES> $<$<TARGET_EXISTS:${APP_NAME_LC}::LibDvdCSS>:-D$<TARGET_PROPERTY:${APP_NAME_LC}::LibDvdCSS,INTERFACE_COMPILE_DEFINITIONS>>")
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

  add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} ${APP_NAME_LC}::LibDvdRead)

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    SETUP_BUILD_TARGET()

    add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_LINK_LIBRARIES ${APP_NAME_LC}::LibDvdRead)

  else()
    if(LibDvdNav_FIND_REQUIRED)
      message(FATAL_ERROR "Libdvdnav not found")
    endif()
  endif()
endif()
