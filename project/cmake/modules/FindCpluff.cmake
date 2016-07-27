# - Builds Cpluff as external project
# Once done this will define
#
# CPLUFF_FOUND - system has cpluff
# CPLUFF_INCLUDE_DIRS - the cpluff include directories
#
# and link Kodi against the cpluff libraries.

if(NOT WIN32)
  string(REPLACE ";" " " defines "${CMAKE_C_FLAGS} ${SYSTEM_DEFINES} -I${EXPAT_INCLUDE_DIR}")
  get_filename_component(expat_dir ${EXPAT_LIBRARY} PATH)
  set(ldflags "-L${expat_dir}")
  ExternalProject_Add(libcpluff SOURCE_DIR ${CORE_SOURCE_DIR}/lib/cpluff
                      BUILD_IN_SOURCE 1
                      PREFIX ${CORE_BUILD_DIR}/cpluff
                      CONFIGURE_COMMAND CC=${CMAKE_C_COMPILER} ${CORE_SOURCE_DIR}/lib/cpluff/configure
                                        --disable-nls
                                        --enable-static
                                        --disable-shared
                                        --with-pic
                                        --prefix=<INSTALL_DIR>
                                        --host=${ARCH}
                                        CFLAGS=${defines}
                                        LDFLAGS=${ldflags})
  ExternalProject_Add_Step(libcpluff autoreconf
                                     DEPENDEES download update patch
                                     DEPENDERS configure
                                     COMMAND rm -f config.status
                                     COMMAND PATH=${NATIVEPREFIX}/bin:$ENV{PATH} autoreconf -vif
                                     WORKING_DIRECTORY <SOURCE_DIR>)

  set(ldflags "${ldflags};-lexpat")
  core_link_library(${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cpluff/lib/libcpluff.a
                    system/libcpluff libcpluff extras "${ldflags}")
  set(CPLUFF_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cpluff/include)
  set(CPLUFF_FOUND 1)
  mark_as_advanced(CPLUFF_INCLUDE_DIRS CPLUFF_FOUND)
else()
  find_path(CPLUFF_INCLUDE_DIR cpluff.h)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CPLUFF
                                    REQUIRED_VARS CPLUFF_INCLUDE_DIR)

  if(CPLUFF_FOUND)
    set(CPLUFF_INCLUDE_DIRS ${CPLUFF_INCLUDE_DIR})
  endif()
  mark_as_advanced(CPLUFF_INCLUDE_DIRS CPLUFF_FOUND)

  add_custom_target(libcpluff)
endif()
set_target_properties(libcpluff PROPERTIES FOLDER "External Projects")
