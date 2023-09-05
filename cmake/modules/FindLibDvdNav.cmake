#.rst:
# FindLibDvdNav
# ----------
# Finds the dvdnav library
#
# This will define the following target:
#
#   LibDvdNav::LibDvdNav   - The LibDvdNav library

if(NOT TARGET LibDvdNav::LibDvdNav)

  # Check for existing LibDvdRead.
  # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
  set(FPHSA_NAME_MISMATCHED 1)
  find_package(LibDvdRead MODULE REQUIRED)
  unset(FPHSA_NAME_MISMATCHED)

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC libdvdnav)

  # We require this due to the odd nature of github URL's compared to our other tarball
  # mirror system. If User sets LIBDVDNAV_URL or libdvdnav_URL, allow get_filename_component in SETUP_BUILD_VARS
  if(LIBDVDNAV_URL OR ${MODULE_LC}_URL)
    if(${MODULE_LC}_URL)
      set(LIBDVDNAV_URL ${${MODULE_LC}_URL})
    endif()
    set(LIBDVDNAV_URL_PROVIDED TRUE)
  endif()

  SETUP_BUILD_VARS()

  if(NOT LIBDVDNAV_URL_PROVIDED)
    # override LIBDVDNAV_URL due to tar naming when retrieved from github release
    set(LIBDVDNAV_URL ${LIBDVDNAV_BASE_URL}/archive/${LIBDVDNAV_VER}.tar.gz)
  endif()

  set(LIBDVDNAV_VERSION ${${MODULE}_VER})

  set(HOST_ARCH ${ARCH})
  if(CORE_SYSTEM_NAME STREQUAL android)
    if(ARCH STREQUAL arm)
      set(HOST_ARCH arm-linux-androideabi)
    elseif(ARCH STREQUAL aarch64)
      set(HOST_ARCH aarch64-linux-android)
    elseif(ARCH STREQUAL i486-linux)
      set(HOST_ARCH i686-linux-android)
    elseif(ARCH STREQUAL x86_64)
      set(HOST_ARCH x86_64-linux-android)
    endif()
  elseif(CORE_SYSTEM_NAME STREQUAL windowsstore)
    set(LIBDVD_ADDITIONAL_ARGS "-DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}" "-DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}")
  endif()

  string(APPEND LIBDVDNAV_CFLAGS "-D_XBMC")

  if(APPLE)
    set(LIBDVDNAV_LDFLAGS "-framework CoreFoundation")
    string(APPEND LIBDVDNAV_CFLAGS " -D__DARWIN__")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      string(APPEND LIBDVDNAV_LDFLAGS " -framework IOKit")
    endif()
  endif()

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS -DDUMMY_DEFINE=ON
                   ${LIBDVD_ADDITIONAL_ARGS})
  else()

    string(APPEND LIBDVDNAV_CFLAGS " -I$<TARGET_PROPERTY:LibDvdRead::LibDvdRead,INTERFACE_INCLUDE_DIRECTORIES>")

    if(TARGET LibDvdCSS::LibDvdCSS)
      string(APPEND LIBDVDNAV_CFLAGS " -I$<TARGET_PROPERTY:LibDvdCSS::LibDvdCSS,INTERFACE_INCLUDE_DIRECTORIES> $<$<TARGET_EXISTS:LibDvdCSS::LibDvdCSS>:-D$<TARGET_PROPERTY:LibDvdCSS::LibDvdCSS,INTERFACE_COMPILE_DEFINITIONS>>")
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
                          "CFLAGS=${CMAKE_C_FLAGS} ${LIBDVDNAV_CFLAGS}"
                          "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} ${LIBDVDNAV_LDFLAGS}"
                          "PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig")

    set(BUILD_COMMAND ${MAKE_EXECUTABLE})
    set(INSTALL_COMMAND ${MAKE_EXECUTABLE} install)
    set(BUILD_IN_SOURCE 1)
  endif()

  BUILD_DEP_TARGET()

  if(TARGET LibDvdRead::LibDvdRead)
    add_dependencies(libdvdnav LibDvdRead::LibDvdRead)
  endif()
endif()

include(SelectLibraryConfigurations)
select_library_configurations(LibDvdNav)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDvdNav
                                  REQUIRED_VARS LIBDVDNAV_LIBRARY LIBDVDNAV_INCLUDE_DIR
                                  VERSION_VAR LIBDVDNAV_VERSION)

if(LIBDVDNAV_FOUND)
  if(NOT TARGET LibDvdNav::LibDvdNav)
    add_library(LibDvdNav::LibDvdNav UNKNOWN IMPORTED)

    set_target_properties(LibDvdNav::LibDvdNav PROPERTIES
                                               IMPORTED_LOCATION "${LIBDVDNAV_LIBRARY}"
                                               INTERFACE_INCLUDE_DIRECTORIES "${LIBDVDNAV_INCLUDE_DIR}")

    if(TARGET libdvdnav)
      add_dependencies(LibDvdNav::LibDvdNav libdvdnav)
    endif()
    if(TARGET LibDvdRead::LibDvdRead)
      add_dependencies(LibDvdNav::LibDvdNav LibDvdRead::LibDvdRead)
    endif()
  endif()
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP LibDvdNav::LibDvdNav)
else()
  if(LIBDVDNAV_FIND_REQUIRED)
    message(FATAL_ERROR "Libdvdnav not found")
  endif()
endif()

mark_as_advanced(LIBDVDNAV_INCLUDE_DIR LIBDVDNAV_LIBRARY)
