cmake_minimum_required(VERSION 3.5)
project(hdhomerun)

set(SOURCES hdhomerun_channels.c
            hdhomerun_channelscan.c
            hdhomerun_control.c
            hdhomerun_debug.c
            hdhomerun_device.c
            hdhomerun_discover.c
            hdhomerun_pkt.c
            hdhomerun_video.c)

set(HEADERS ${PROJECT_SOURCE_DIR}/hdhomerun.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_channels.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_channelscan.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_control.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_debug.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_device.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_device_selector.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_discover.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_pkt.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_sock.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_types.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_os.h
            ${PROJECT_SOURCE_DIR}/hdhomerun_video.h)

if(WIN32)
  LIST(APPEND SOURCES hdhomerun_os_windows.c)
  LIST(APPEND SOURCES hdhomerun_sock_windows.c)
  LIST(APPEND HEADERS ${PROJECT_SOURCE_DIR}/hdhomerun_os_windows.h)
else()
  LIST(APPEND SOURCES hdhomerun_os_posix.c)
  LIST(APPEND SOURCES hdhomerun_sock_posix.c)
  LIST(APPEND HEADERS ${PROJECT_SOURCE_DIR}/hdhomerun_os_posix.h)
endif()

if(MSVC)
  set_source_files_properties( ${SOURCES} PROPERTIES LANGUAGE "CXX" )
  add_definitions(-D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
  if(CMAKE_SYSTEM_NAME STREQUAL WindowsStore)
    add_definitions(-D_WIN32_WINNT=0x0A00 -D_WINRT)
  endif()
endif()

add_library(hdhomerun ${SOURCES})

include_directories(${PROJECT_SOURCE_DIR})

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/hdhomerun)
install(TARGETS hdhomerun DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
