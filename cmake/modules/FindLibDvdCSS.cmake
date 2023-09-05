#.rst:
# FindLibDvdCSS
# ----------
# Finds the libdvdcss library
#
# This will define the following target:
#
#   LibDvdCSS::LibDvdCSS   - The LibDvdCSS library

if(ENABLE_DVDCSS)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC libdvdcss)

  # We require this due to the odd nature of github URL's compared to our other tarball
  # mirror system. If User sets LIBDVDCSS_URL or libdvdcss_URL, allow get_filename_component in SETUP_BUILD_VARS
  if(LIBDVDCSS_URL OR ${MODULE_LC}_URL)
    if(${MODULE_LC}_URL)
      set(LIBDVDCSS_URL ${${MODULE_LC}_URL})
    endif()
    set(LIBDVDCSS_URL_PROVIDED TRUE)
  endif()

  SETUP_BUILD_VARS()

  if(NOT LIBDVDCSS_URL_PROVIDED)
    # override LIBDVDCSS_URL_PROVIDED due to tar naming when retrieved from github release
    set(LIBDVDCSS_URL ${LIBDVDCSS_BASE_URL}/archive/${LIBDVDCSS_VER}.tar.gz)
  endif()

  set(LIBDVDCSS_VERSION ${${MODULE}_VER})

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
    set(${MODULE}_CXX_FLAGS "/Zc:twoPhase-")
  endif()

  if(APPLE)
    set(LIBDVDCSS_FLAGS " -framework CoreFoundation")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      string(APPEND LIBDVDCSS_FLAGS " -framework IOKit")
    endif()
  endif()

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS -DDUMMY_DEFINE=ON
                   ${LIBDVD_ADDITIONAL_ARGS})
  else()
    find_program(AUTORECONF autoreconf REQUIRED)
    if (CMAKE_HOST_SYSTEM_NAME MATCHES "(Free|Net|Open)BSD")
      find_program(MAKE_EXECUTABLE gmake)
    endif()
    find_program(MAKE_EXECUTABLE make REQUIRED)

    set(CONFIGURE_COMMAND ${AUTORECONF} -vif
                  COMMAND ac_cv_path_GIT= ./configure
                          --target=${HOST_ARCH}
                          --host=${HOST_ARCH}
                          --disable-doc
                          --enable-static
                          --disable-shared
                          --with-pic
                          --prefix=${DEPENDS_PATH}
                          --libdir=${DEPENDS_PATH}/lib
                          "CC=${CMAKE_C_COMPILER}"
                          "CFLAGS=${CMAKE_C_FLAGS}"
                          "LDFLAGS=${CMAKE_EXE_LINKER_FLAGS} ${LIBDVDCSS_FLAGS}")
    set(BUILD_COMMAND ${MAKE_EXECUTABLE})
    set(INSTALL_COMMAND ${MAKE_EXECUTABLE} install)
    set(BUILD_IN_SOURCE 1)
  endif()

  BUILD_DEP_TARGET()

endif()

include(SelectLibraryConfigurations)
select_library_configurations(LibDvdCSS)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDvdCSS
                                  REQUIRED_VARS LIBDVDCSS_LIBRARY LIBDVDCSS_INCLUDE_DIR
                                  VERSION_VAR LIBDVDCSS_VERSION)

if(LIBDVDCSS_FOUND)
  if(NOT TARGET LibDvdCSS::LibDvdCSS)
    add_library(LibDvdCSS::LibDvdCSS UNKNOWN IMPORTED)

    set_target_properties(LibDvdCSS::LibDvdCSS PROPERTIES
                                               IMPORTED_LOCATION "${LIBDVDCSS_LIBRARY}"
                                               INTERFACE_COMPILE_DEFINITIONS "HAVE_DVDCSS_DVDCSS_H"
                                               INTERFACE_INCLUDE_DIRECTORIES "${LIBDVDCSS_INCLUDE_DIR}")

    if(TARGET libdvdcss)
      add_dependencies(LibDvdCSS::LibDvdCSS libdvdcss)
    endif()
  endif()

else()
  if(LIBDVDCSS_FIND_REQUIRED)
    message(FATAL_ERROR "Libdvdcss not found. Possibly remove ENABLE_DVDCSS.")
  endif()
endif()

mark_as_advanced(LIBDVDCSS_INCLUDE_DIR LIBDVDCSS_LIBRARY)
