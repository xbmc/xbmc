if(NOT WIN32)
  if(CMAKE_CROSSCOMPILING)
    set(EXTRA_FLAGS "CC=${CMAKE_C_COMPILER}")
  endif()

  if(APPLE)
      set(CMAKE_LD_FLAGS "-framework IOKit -framework CoreFoundation")
  endif()

  if(ENABLE_DVDCSS)
    ExternalProject_ADD(dvdcss SOURCE_DIR ${CORE_SOURCE_DIR}/lib/libdvd/libdvdcss/
                               PREFIX ${CORE_BUILD_DIR}/libdvd
                        PATCH_COMMAND rm -f config.status
                        UPDATE_COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                        CONFIGURE_COMMAND  <SOURCE_DIR>/configure
                                          --target=${ARCH}
                                          --host=${ARCH}
                                          --disable-doc
                                          --enable-static
                                          --disable-shared
                                          --with-pic
                                          --prefix=<INSTALL_DIR>
                                          "${EXTRA_FLAGS}"
                                          "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                          "LDFLAGS=${CMAKE_LD_FLAGS}")

    core_link_library(${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdcss.a
                      system/players/VideoPlayer/libdvdcss dvdcss)
  endif()

  set(DVDREAD_CFLAGS "-D_XBMC")
  if(ENABLE_DVDCSS)
    set(DVDREAD_CFLAGS "${DVDREAD_CFLAGS} -DHAVE_DVDCSS_DVDCSS_H -I${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include")
  endif(ENABLE_DVDCSS)

  ExternalProject_ADD(dvdread SOURCE_DIR ${CORE_SOURCE_DIR}/lib/libdvd/libdvdread/
                              PREFIX ${CORE_BUILD_DIR}/libdvd
                      PATCH_COMMAND rm -f config.status
                      UPDATE_COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                      CONFIGURE_COMMAND <SOURCE_DIR>/configure
                                        --target=${ARCH}
                                        --host=${ARCH}
                                        --enable-static
                                        --disable-shared
                                        --with-pic
                                        --prefix=<INSTALL_DIR>
                                        "${EXTRA_FLAGS}"
                                        "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                        "LDFLAGS=${CMAKE_LD_FLAGS}")
  if(ENABLE_DVDCSS)
    add_dependencies(dvdread dvdcss)
  endif()

  core_link_library(${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.a
                    system/players/VideoPlayer/libdvdread dvdread)

  ExternalProject_ADD(dvdnav SOURCE_DIR ${CORE_SOURCE_DIR}/lib/libdvd/libdvdnav/
                             PREFIX ${CORE_BUILD_DIR}/libdvd
                      PATCH_COMMAND rm -f config.status
                      UPDATE_COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                      CONFIGURE_COMMAND <SOURCE_DIR>/configure
                                        --target=${ARCH}
                                        --host=${ARCH}
                                        --disable-shared
                                        --enable-static
                                        --with-pic
                                        --prefix=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd
                                        --with-dvdread-config=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/bin/dvdread-config
                                        "${EXTRA_FLAGS}"
                                        "LDFLAGS=${CMAKE_LD_FLAGS} -L${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib"
                                        "CFLAGS=${CMAKE_C_FLAGS} ${DVDREAD_CFLAGS}"
                                        "LIBS=-ldvdcss")
  add_dependencies(dvdnav dvdread)
  core_link_library(${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.a
                    system/players/VideoPlayer/libdvdnav dvdnav)

  set(dvdnav_internal_headers ${CORE_SOURCE_DIR}/lib/libdvd/libdvdnav/src/dvdnav_internal.h
                              ${CORE_SOURCE_DIR}/lib/libdvd/libdvdnav/src/vm/vm.h)

  foreach(dvdnav_header ${dvdnav_internal_headers})
   add_custom_command(TARGET dvdnav
                     COMMAND ${CMAKE_COMMAND} -E copy ${dvdnav_header}
                             ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include/dvdnav)
  endforeach()

  if(ENABLE_DVDCSS)
    set(LIBDVD_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/include)
    set(LIBDVD_LIBRARIES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdnav.a
                         ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdread.a
                         ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/libdvd/lib/libdvdcss.a
        CACHE STRING "libdvd libraries" FORCE)
    set(LIBDVD_FOUND 1 CACHE BOOL "libdvd found" FORCE)
  endif()
else()
  # Dynamically loaded on Windows
  find_path(LIBDVD_INCLUDE_DIR dvdcss/dvdcss.h
                               PATH_SUFFIXES libdvd/includes
                               PATHS ${PC_BLURAY_INCLUDEDIR})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LIBDVD
                                    REQUIRED_VARS LIBDVD_INCLUDE_DIR)

  if(LIBDVD_FOUND)
    set(LIBDVD_INCLUDE_DIRS ${LIBDVD_INCLUDE_DIR})
  endif()

  mark_as_advanced(LIBDVD_INCLUDE_DIR)
endif()
