cmake_minimum_required(VERSION 3.0)

project(cpluff VERSION 0.1.3 LANGUAGES C)
set(CP_VERSION_MAJOR 0)
set(CP_VERSION_MINOR 1)

find_package(expat 2.2.0 NO_MODULE)

set(KAZDIR ${CMAKE_CURRENT_SOURCE_DIR}/kazlib)
add_library(kazlib STATIC
  ${KAZDIR}/hash.h
  ${KAZDIR}/hash.c
  ${KAZDIR}/list.h
  ${KAZDIR}/list.c
)

set(CPLUFFDIR ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff)
configure_file(
  ${CPLUFFDIR}/cpluffdef.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/cpluffdef.h
  @ONLY
)

add_library(cpluff
  ${CPLUFFDIR}/context.c
  ${CPLUFFDIR}/cpluff.h
  ${CPLUFFDIR}/cpluff.c
  ${CPLUFFDIR}/defines.h
  ${CPLUFFDIR}/internal.h
  ${CPLUFFDIR}/logging.c
  ${CPLUFFDIR}/pcontrol.c
  ${CPLUFFDIR}/pinfo.c
  ${CPLUFFDIR}/ploader.c
  ${CPLUFFDIR}/pscan.c
  ${CPLUFFDIR}/psymbol.c
  ${CPLUFFDIR}/serial.c
  ${CPLUFFDIR}/thread.h
  ${CPLUFFDIR}/util.c
  ${CPLUFFDIR}/util.h
  ${CMAKE_CURRENT_BINARY_DIR}/cpluffdef.h
)

if(WIN32)
  target_sources(cpluff
    PRIVATE  
    ${CPLUFFDIR}/win32/dirent.c
    ${CPLUFFDIR}/win32/dirent.h
    ${CPLUFFDIR}/thread_windows.c
  )
  target_compile_definitions(cpluff
    PRIVATE
    XML_STATIC
    _USRDLL
    CP_C_API=CP_EXPORT
    _CRT_SECURE_NO_WARNINGS
    _CRT_NONSTDC_NO_DEPRECATE
  )
else()
  target_sources(cpluff
    PRIVATE
    ${CPLUFFDIR}/thread_posix.c
  )
endif()

target_link_libraries(cpluff PRIVATE kazlib expat::expat)
target_include_directories(cpluff
  PRIVATE
  $<BUILD_INTERFACE:kazlib;libcpluff;libcpluff/win32>
  INTERFACE
  $<INSTALL_INTERFACE:include>
)


include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/cpluff-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(TARGETS cpluff kazlib EXPORT cpluff 
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/cpluffdef.h
  ${CPLUFFDIR}/cpluff.h
  DESTINATION include
)

install(EXPORT cpluff
  FILE
    cpluff.cmake
  NAMESPACE
    cpluff::
  DESTINATION
    lib/cmake/cpluff
)
install(
  FILES
    cmake/cpluff-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cpluff-config-version.cmake
  DESTINATION
    lib/cmake/cpluff
)