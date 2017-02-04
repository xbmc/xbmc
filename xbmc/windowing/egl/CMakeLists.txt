if (NOT MIR_FOUND)
  if(OPENGLES_FOUND OR AML_FOUND OR IMX_FOUND OR MMAL_FOUND)
    set(SOURCES EGLWrapper.cpp)

    set(HEADERS EGLNativeType.h
                EGLQuirks.h
                EGLWrapper.h)
  endif()

  if(OPENGLES_FOUND)
    list(APPEND SOURCES WinSystemEGL.cpp)
    list(APPEND HEADERS WinSystemEGL.h)
  endif()
endif()

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES EGLNativeTypeAndroid.cpp)
  list(APPEND HEADERS EGLNativeTypeAndroid.h)
endif()

if(AML_FOUND)
  list(APPEND SOURCES EGLNativeTypeAmlogic.cpp)
  list(APPEND HEADERS EGLNativeTypeAmlogic.h)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES EGLNativeTypeRaspberryPI.cpp)
  list(APPEND HEADERS EGLNativeTypeRaspberryPI.h)
  list(APPEND SOURCES VideoSyncPi.cpp)
  list(APPEND HEADERS VideoSyncPi.h)
endif()

if(IMX_FOUND)
  list(APPEND SOURCES EGLNativeTypeIMX.cpp)
  list(APPEND HEADERS EGLNativeTypeIMX.h)
endif()

if(SOURCES)
  core_add_library(windowing_egl)
endif()
