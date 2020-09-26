set(SOURCES DRMAtomic.cpp
            DRMLegacy.cpp
            DRMUtils.cpp
            OffScreenModeSetting.cpp)

set(HEADERS DRMAtomic.h
            DRMLegacy.h
            DRMUtils.h
            OffScreenModeSetting.h)

core_add_library(windowing_gbm_drm)
