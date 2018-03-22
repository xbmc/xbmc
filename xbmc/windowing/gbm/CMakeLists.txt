set(SOURCES OptionalsReg.cpp
            WinSystemGbm.cpp
            GBMUtils.cpp
            DRMUtils.cpp
            DRMLegacy.cpp
            DRMAtomic.cpp)

set(HEADERS OptionalsReg.h
            WinSystemGbm.h
            GBMUtils.h
            DRMUtils.h
            DRMLegacy.h
            DRMAtomic.h)

if(OPENGLES_FOUND)
  list(APPEND SOURCES WinSystemGbmGLESContext.cpp)
  list(APPEND HEADERS WinSystemGbmGLESContext.h)
endif()

core_add_library(windowing_Gbm)
