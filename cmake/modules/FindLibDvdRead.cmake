#.rst:
# FindLibDvdRead
# ----------
# Finds the dvdread library
#
# This will define the following target:
#
#   LibDvdRead::LibDvdRead   - The LibDvdRead library

if(NOT TARGET LibDvdRead::LibDvdRead)

  if(ENABLE_DVDCSS)
    # Check for existing LIBDVDCSS.
    # Suppress mismatch warning, see https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    set(FPHSA_NAME_MISMATCHED 1)
    find_package(LibDvdCSS MODULE REQUIRED)
    unset(FPHSA_NAME_MISMATCHED)
  endif()

  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC libdvdread)

  # We require this due to the odd nature of github URL's compared to our other tarball
  # mirror system. If User sets LIBDVDREAD_URL or libdvdread_URL, allow get_filename_component in SETUP_BUILD_VARS
  if(LIBDVDREAD_URL OR ${MODULE_LC}_URL)
    if(${MODULE_LC}_URL)
      set(LIBDVDREAD_URL ${${MODULE_LC}_URL})
    endif()
    set(LIBDVDREAD_URL_PROVIDED TRUE)
  endif()

  SETUP_BUILD_VARS()

  if(NOT LIBDVDREAD_URL_PROVIDED)
    # override LIBDVDREAD_URL due to tar naming when retrieved from github release
    set(LIBDVDREAD_URL ${LIBDVDREAD_BASE_URL}/archive/${LIBDVDREAD_VER}.tar.gz)
  endif()

  set(LIBDVDREAD_VERSION ${${MODULE}_VER})

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

  string(APPEND LIBDVDREAD_CFLAGS "-D_XBMC")

  if(APPLE)
    set(LIBDVDREAD_LDFLAGS "-framework CoreFoundation")
    string(APPEND LIBDVDREAD_CFLAGS " -D__DARWIN__")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      string(APPEND LIBDVDREAD_LDFLAGS " -framework IOKit")
    endif()
  endif()

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS -DDUMMY_DEFINE=ON
                   ${LIBDVD_ADDITIONAL_ARGS})
  else()

    if(TARGET LibDvdCSS::LibDvdCSS)
      string(APPEND LIBDVDREAD_CFLAGS " -I$<TARGET_PROPERTY:LibDvdCSS::LibDvdCSS,INTERFACE_INCLUDE_DIRECTORIES> $<$<TARGET_EXISTS:LibDvdCSS::LibDvdCSS>:-D$<TARGET_PROPERTY:LibDvdCSS::LibDvdCSS,INTERFACE_COMPILE_DEFINITIONS>>")
      string(APPEND with-css "--with-libdvdcss")
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
                          ${with-css}
                          "CC=${CMAKE_C_COMPILER}"
                          "CFLAGS=${CMAKE_C_FLAGS} ${LIBDVDREAD_CFLAGS}"
                          "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} ${LIBDVDREAD_LDFLAGS}"
                          "PKG_CONFIG_PATH=${DEPENDS_PATH}/lib/pkgconfig")

    set(BUILD_COMMAND ${MAKE_EXECUTABLE})
    set(INSTALL_COMMAND ${MAKE_EXECUTABLE} install)
    set(BUILD_IN_SOURCE 1)
  endif()

  BUILD_DEP_TARGET()

  if(TARGET LibDvdCSS::LibDvdCSS)
    add_dependencies(libdvdread LibDvdCSS::LibDvdCSS)
  endif()
endif()

include(SelectLibraryConfigurations)
select_library_configurations(LibDvdRead)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDvdRead
                                  REQUIRED_VARS LIBDVDREAD_LIBRARY LIBDVDREAD_INCLUDE_DIR
                                  VERSION_VAR LIBDVDREAD_VERSION)

if(LIBDVDREAD_FOUND)
  if(NOT TARGET LibDvdRead::LibDvdRead)
    add_library(LibDvdRead::LibDvdRead UNKNOWN IMPORTED)

    set_target_properties(LibDvdRead::LibDvdRead PROPERTIES
                                                 IMPORTED_LOCATION "${LIBDVDREAD_LIBRARY}"
                                                 INTERFACE_INCLUDE_DIRECTORIES "${LIBDVDREAD_INCLUDE_DIR}")

    if(TARGET libdvdread)
      add_dependencies(LibDvdRead::LibDvdRead libdvdread)
    endif()
    if(TARGET LibDvdCSS::LibDvdCSS)
      add_dependencies(LibDvdRead::LibDvdRead LibDvdCSS::LibDvdCSS)
      set_target_properties(LibDvdRead::LibDvdRead PROPERTIES
                                                   INTERFACE_LINK_LIBRARIES "dvdcss")
    endif()
  endif()

else()
  if(LIBDVDREAD_FIND_REQUIRED)
    message(FATAL_ERROR "Libdvdread not found")
  endif()
endif()

mark_as_advanced(LIBDVDREAD_INCLUDE_DIR LIBDVDREAD_LIBRARY)
