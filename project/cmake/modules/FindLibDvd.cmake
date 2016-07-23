set(dvdlibs libdvdread libdvdnav)
if(ENABLE_DVDCSS)
  list(APPEND dvdlibs libdvdcss)
endif()

if(NOT WIN32)
  foreach(dvdlib ${dvdlibs})
    file(GLOB VERSION_FILE ${CORE_SOURCE_DIR}/tools/depends/target/${dvdlib}/DVD*-VERSION)
    file(STRINGS ${VERSION_FILE} VER)
    string(REGEX MATCH "VERSION=[^ ]*$.*" ${dvdlib}_VER "${VER}")
    list(GET ${dvdlib}_VER 0 ${dvdlib}_VER)
    string(SUBSTRING "${${dvdlib}_VER}" 8 -1 ${dvdlib}_VER)
    string(REGEX MATCH "BASE_URL=([^ ]*)" ${dvdlib}_BASE_URL "${VER}")
    list(GET ${dvdlib}_BASE_URL 0 ${dvdlib}_BASE_URL)
    string(SUBSTRING "${${dvdlib}_BASE_URL}" 9 -1 ${dvdlib}_BASE_URL)
  endforeach()

  set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")
  if(CMAKE_CROSSCOMPILING)
    set(EXTRA_FLAGS "CC=${CMAKE_C_COMPILER}")
  endif()

  if(APPLE)
    set(CMAKE_LD_FLAGS "-framework IOKit -framework CoreFoundation")
  endif()

  set(HOST_ARCH ${ARCH})
  if(CORE_SYSTEM_NAME STREQUAL android)
    if(ARCH STREQUAL arm)
      set(HOST_ARCH arm-linux-androideabi)
    elseif(ARCH STREQUAL i486-linux)
      set(HOST_ARCH i686-linux-android)
    endif()
  endif()

  if(ENABLE_DVDCSS)
    set(DVDCSS_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdcss.a)
    ExternalProject_Add(dvdcss URL ${libdvdcss_BASE_URL}/archive/${libdvdcss_VER}.tar.gz
                               PREFIX ${CORE_BUILD_DIR}/libdvd
                               CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
                                                 --target=${HOST_ARCH}
                                                 --host=${HOST_ARCH}
                                                 --disable-doc
                                                 --enable-static
                                                 --disable-shared
                                                 --with-pic
                                                 --prefix=<INSTALL_DIR>
                                                 "${EXTRA_FLAGS}"
                                                 "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                                 "LDFLAGS=${CMAKE_LD_FLAGS}"
                               BUILD_BYPRODUCTS ${DVDCSS_LIBRARY})
    ExternalProject_Add_Step(dvdcss autoreconf
                                    DEPENDEES download update patch
                                    DEPENDERS configure
                                    COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                    WORKING_DIRECTORY <SOURCE_DIR>)

    set_target_properties(dvdcss PROPERTIES FOLDER "External Projects")
    core_link_library(${DVDCSS_LIBRARY} system/players/VideoPlayer/libdvdcss dvdcss)
  endif()

  set(DVDREAD_CFLAGS "-D_XBMC")
  if(ENABLE_DVDCSS)
    set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -DHAVE_DVDCSS_DVDCSS_H -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")
  endif()

  set(DVDREAD_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.a)
  ExternalProject_Add(dvdread URL ${libdvdread_BASE_URL}/archive/${libdvdread_VER}.tar.gz
                              PREFIX ${CORE_BUILD_DIR}/libdvd
                              CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
                                                --target=${HOST_ARCH}
                                                --host=${HOST_ARCH}
                                                --enable-static
                                                --disable-shared
                                                --with-pic
                                                --prefix=<INSTALL_DIR>
                                                "${EXTRA_FLAGS}"
                                                "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                                "LDFLAGS=${CMAKE_LD_FLAGS}"
                              BUILD_BYPRODUCTS ${DVDREAD_LIBRARY})
  ExternalProject_Add_Step(dvdread autoreconf
                                   DEPENDEES download update patch
                                   DEPENDERS configure
                                   COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                   WORKING_DIRECTORY <SOURCE_DIR>)
  if(ENABLE_DVDCSS)
    add_dependencies(dvdread dvdcss)
  endif()

  set_target_properties(dvdread PROPERTIES FOLDER "External Projects")
  core_link_library(${DVDREAD_LIBRARY} system/players/VideoPlayer/libdvdread dvdread)

  if(ENABLE_DVDCSS)
    set(DVDNAV_LIBS -ldvdcss)
  endif()

  set(DVDNAV_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.a)
  ExternalProject_Add(dvdnav URL ${libdvdnav_BASE_URL}/archive/${libdvdnav_VER}.tar.gz
                             PREFIX ${CORE_BUILD_DIR}/libdvd
                             CONFIGURE_COMMAND ac_cv_path_GIT= <SOURCE_DIR>/configure
                                               --target=${HOST_ARCH}
                                               --host=${HOST_ARCH}
                                               --enable-static
                                               --disable-shared
                                               --with-pic
                                               --prefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
                                               "${EXTRA_FLAGS}"
                                               "LDFLAGS=${CMAKE_LD_FLAGS} -L${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib"
                                               "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                               "DVDREAD_CFLAGS=${DVDREAD_CFLAGS}"
                                               "DVDREAD_LIBS=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.la"
                                               "LIBS=${DVDNAV_LIBS}"
                             BUILD_BYPRODUCTS ${DVDNAV_LIBRARY})
  ExternalProject_Add_Step(dvdnav autoreconf
                                  DEPENDEES download update patch
                                  DEPENDERS configure
                                  COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                  WORKING_DIRECTORY <SOURCE_DIR>)
  add_dependencies(dvdnav dvdread)
  set_target_properties(dvdnav PROPERTIES FOLDER "External Projects")
  core_link_library(${DVDNAV_LIBRARY} system/players/VideoPlayer/libdvdnav dvdnav)

  set(LIBDVD_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include)
  set(LIBDVD_LIBRARIES ${DVDNAV_LIBRARY} ${DVDREAD_LIBRARY})
  if(ENABLE_DVDCSS)
    list(APPEND LIBDVD_LIBRARIES ${DVDCSS_LIBRARY})
  endif()
  set(LIBDVD_LIBRARIES ${LIBDVD_LIBRARIES} CACHE STRING "libdvd libraries" FORCE)
  set(LIBDVD_FOUND 1 CACHE BOOL "libdvd found" FORCE)
else()
  # Dynamically loaded on Windows
  find_path(LIBDVD_INCLUDE_DIR dvdcss/dvdcss.h PATHS ${CORE_SOURCE_DIR}/lib/libdvd/include)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LIBDVD REQUIRED_VARS LIBDVD_INCLUDE_DIR)

  if(LIBDVD_FOUND)
    set(LIBDVD_INCLUDE_DIRS ${LIBDVD_INCLUDE_DIR})

    add_custom_target(dvdnav)
    set_target_properties(dvdnav PROPERTIES FOLDER "External Projects")
  endif()

  mark_as_advanced(LIBDVD_INCLUDE_DIR)
endif()
