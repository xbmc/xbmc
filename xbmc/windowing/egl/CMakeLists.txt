set(SOURCES EGLWrapper.cpp
            WinSystemEGL.cpp)

set(HEADERS EGLNativeType.h
            EGLQuirks.h
            EGLWrapper.h
            WinSystemEGL.h)


if(MMAL_FOUND)
  list(APPEND SOURCES EGLNativeTypeRaspberryPI.cpp)
  list(APPEND HEADERS EGLNativeTypeRaspberryPI.h)
endif()

core_add_library(windowing_egl)
