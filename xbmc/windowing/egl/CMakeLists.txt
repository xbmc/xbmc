set(SOURCES EGLWrapper.cpp
            WinSystemEGL.cpp)

set(HEADERS EGLNativeType.h
            EGLQuirks.h
            EGLWrapper.h
            WinSystemEGL.h)

if(CORE_SYSTEM_NAME STREQUAL android)
  list(APPEND SOURCES EGLNativeTypeAndroid.cpp
                      EGLNativeTypeRKAndroid.cpp)
  list(APPEND HEADERS EGLNativeTypeAndroid.h
                      EGLNativeTypeRKAndroid.h)
  if(AML_FOUND)
    list(APPEND SOURCES EGLNativeTypeAmlAndroid.cpp)
    list(APPEND HEADERS EGLNativeTypeAmlAndroid.h)
  endif()
endif()

if(AML_FOUND)
  list(APPEND SOURCES EGLNativeTypeAmlogic.cpp)
  list(APPEND HEADERS EGLNativeTypeAmlogic.h)
endif()

if(MMAL_FOUND)
  list(APPEND SOURCES EGLNativeTypeRaspberryPI.cpp)
  list(APPEND HEADERS EGLNativeTypeRaspberryPI.h)
endif()

core_add_library(windowing_egl)
