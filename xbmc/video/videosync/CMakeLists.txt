set(HEADERS VideoSync.h)

if(OPENGL_FOUND)
  list(APPEND SOURCES VideoSyncGLX.cpp)
  list(APPEND HEADERS VideoSyncGLX.h)
endif()

if(X_FOUND)
  list(APPEND SOURCES VideoSyncDRM.cpp)
  list(APPEND HEADERS VideoSyncDRM.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES VideoSyncD3D.cpp)
  list(APPEND HEADERS VideoSyncD3D.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL rbpi)
  list(APPEND SOURCES VideoSyncPi.cpp)
  list(APPEND HEADERS VideoSyncPi.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL darwin)
  list(APPEND SOURCES VideoSyncOsx.cpp)
  list(APPEND HEADERS VideoSyncOsx.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL ios)
  list(APPEND SOURCES VideoSyncIos.cpp)
  list(APPEND HEADERS VideoSyncIos.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES VideoSyncAndroid.cpp)
  list(APPEND HEADERS VideoSyncAndroid.h)
endif()

if(SOURCES AND HEADERS)
  core_add_library(video_sync)
  if(CORE_SYSTEM_NAME STREQUAL windows)
    add_dependencies(video_sync d3dx11effects)
  endif()
endif()
