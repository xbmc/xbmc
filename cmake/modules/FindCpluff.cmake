# - Builds Cpluff as external project
# Once done this will define
#
# CPLUFF_FOUND - system has cpluff
# CPLUFF_INCLUDE_DIRS - the cpluff include directories
#
# and link Kodi against the cpluff libraries.

find_package(EXPAT REQUIRED)
if(CORE_SYSTEM_NAME MATCHES windows)
  add_subdirectory(${CMAKE_SOURCE_DIR}/lib/cpluff)
  set(CPLUFF_LIBRARIES $<TARGET_FILE:libcpluff> ${EXPAT_LIBRARIES})
else()
  string(REPLACE ";" " " defines "${CMAKE_C_FLAGS} ${SYSTEM_DEFINES} -I${EXPAT_INCLUDE_DIR}")
  get_filename_component(expat_dir ${EXPAT_LIBRARY} DIRECTORY)
  set(ldflags "-L${expat_dir}")

  # iOS: Without specifying -arch, configure tries to use /bin/cpp as C-preprocessor
  # http://stackoverflow.com/questions/38836754/cant-cross-compile-c-library-for-arm-ios
  if(CORE_SYSTEM_NAME STREQUAL ios)
    set(cppflags "-arch ${CPU}")
  endif()

  ExternalProject_Add(libcpluff SOURCE_DIR ${CMAKE_SOURCE_DIR}/lib/cpluff
                      BUILD_IN_SOURCE 1
                      PREFIX ${CORE_BUILD_DIR}/cpluff
                      CONFIGURE_COMMAND AR=${CMAKE_AR} RANLIB=${CMAKE_RANLIB} CC=${CMAKE_C_COMPILER} ${CMAKE_SOURCE_DIR}/lib/cpluff/configure
                                        --disable-nls
                                        --enable-static
                                        --disable-shared
                                        --with-pic
                                        --prefix=<INSTALL_DIR>
                                        --libdir=<INSTALL_DIR>/lib
                                        --host=${ARCH}
                                        CFLAGS=${defines}
                                        CPPFLAGS=${cppflags}
                                        LDFLAGS=${ldflags}
                      BUILD_BYPRODUCTS <INSTALL_DIR>/lib/libcpluff.a)
  ExternalProject_Add_Step(libcpluff autoreconf
                                     DEPENDEES download update patch
                                     DEPENDERS configure
                                     COMMAND rm -f config.status
                                     COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                     WORKING_DIRECTORY <SOURCE_DIR>)

  set(CPLUFF_LIBRARIES ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cpluff/lib/libcpluff.a ${EXPAT_LIBRARIES})
endif()
set(CPLUFF_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cpluff/include)
set(CPLUFF_FOUND 1)
mark_as_advanced(CPLUFF_INCLUDE_DIRS CPLUFF_LIBRARIES)
set_target_properties(libcpluff PROPERTIES FOLDER "External Projects")
