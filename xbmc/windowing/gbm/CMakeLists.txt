set(SOURCES GLContextEGL.cpp
            WinSystemGbm.cpp
            GBMUtils.cpp)

set(HEADERS GLContextEGL.h
            WinSystemGbm.h
            GBMUtils.h)

if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemGbmGLESContext.cpp)
  list(APPEND HEADERS WinSystemGbmGLESContext.h)
endif()

core_add_library(windowing_Gbm)
