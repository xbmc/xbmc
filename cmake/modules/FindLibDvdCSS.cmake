#.rst:
# FindLibDvdCSS
# ----------
# Finds the libdvdcss library
#
# This will define the following target:
#
#   LibDvdCSS::LibDvdCSS   - The LibDvdCSS library

if(NOT TARGET LibDvdCSS::LibDvdCSS)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libdvdcss)

  SETUP_BUILD_VARS()

  # Legacy support for lowercase user provided URL override
  if(libdvdcss_URL)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_URL ${libdvdcss_URL})
  endif()

  set(LIBDVDCSS_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})

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
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_CXX_FLAGS "/Zc:twoPhase-")
  endif()

  if(APPLE)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "-framework CoreFoundation")
    if(NOT CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES "-framework IOKit")
    endif()
    string(REPLACE ";" " " LIBDVDCSS_FLAGS "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES}")
  endif()

  if(CORE_SYSTEM_NAME MATCHES windows)
    set(CMAKE_ARGS -DDUMMY_DEFINE=ON
                   -DCMAKE_POLICY_VERSION_MINIMUM=3.5
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

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    SETUP_BUILD_TARGET()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_DVDCSS_DVDCSS_H)
    ADD_TARGET_COMPILE_DEFINITION()

    add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
  else()
    if(LibDvdCSS_FIND_REQUIRED)
      message(FATAL_ERROR "Libdvdcss not found. Possibly remove ENABLE_DVDCSS.")
    endif()
  endif()
endif()
