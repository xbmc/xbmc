set(SOURCES DRMAtomic.cpp
            DRMConnector.cpp
            DRMCrtc.cpp
            DRMEncoder.cpp
            DRMLegacy.cpp
            DRMObject.cpp
            DRMPlane.cpp
            DRMUtils.cpp
            OffScreenModeSetting.cpp)

set(HEADERS DRMAtomic.h
            DRMConnector.h
            DRMCrtc.h
            DRMEncoder.h
            DRMLegacy.h
            DRMObject.h
            DRMPlane.h
            DRMUtils.h
            OffScreenModeSetting.h)

core_add_library(windowing_gbm_drm)
