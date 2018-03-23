cmake_minimum_required(VERSION 3.0)

project(libcpluff VERSION 0.1.4 LANGUAGES C)

find_package(expat 2.2.0 REQUIRED)

add_library(${PROJECT_NAME}
  ${CMAKE_CURRENT_SOURCE_DIR}/kazlib/hash.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kazlib/hash.c
  ${CMAKE_CURRENT_SOURCE_DIR}/kazlib/list.h
  ${CMAKE_CURRENT_SOURCE_DIR}/kazlib/list.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/context.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/cpluff.h
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/cpluff.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/defines.h
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/internal.h
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/logging.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/pcontrol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/pinfo.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/ploader.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/pscan.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/psymbol.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/serial.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/thread.h
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/util.h
  ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/util.c
)

if(WIN32)
  target_sources(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/thread_windows.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/win32/cpluffdef.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/win32/dirent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/win32/dirent.c
    ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/win32/win32_utils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libcpluff/win32/win32_utils.c
  )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE ${EXPAT_LIBRARIES})

target_include_directories(${PROJECT_NAME}
  PRIVATE
  $<BUILD_INTERFACE:kazlib;libcpluff;libcpluff/win32;${EXPAT_INCLUDE_DIR}>
  INTERFACE
  $<INSTALL_INTERFACE:include>
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
  XML_STATIC
  CP_C_API=CP_EXPORT
  _CRT_SECURE_NO_WARNINGS
  _CRT_NONSTDC_NO_DEPRECATE
)
