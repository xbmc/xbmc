if (NOT MIR_FOUND AND NOT X_FOUND AND NOT MMAL_FOUND AND NOT AML_FOUND AND NOT GBM_FOUND)
  if(OPENGLES_FOUND OR AML_FOUND OR IMX_FOUND)
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
  list(APPEND SOURCES VideoSyncAndroid.h)
  list(APPEND SOURCES VideoSyncAndroid.cpp)
endif()

if(IMX_FOUND)
  list(APPEND SOURCES EGLNativeTypeIMX.cpp)
  list(APPEND HEADERS EGLNativeTypeIMX.h)
endif()

if(SOURCES)
  core_add_library(windowing_egl)
endif()
