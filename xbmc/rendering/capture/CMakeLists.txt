set(SOURCES CaptureBlit.cpp
            CaptureConvert.cpp
            CaptureHandle.cpp
            CaptureMetadata.cpp
            CaptureReadback.cpp
            CaptureService.cpp)

set(HEADERS CaptureBlit.h
            CaptureConvert.h
            CaptureHandle.h
            CaptureMetadata.h
            CaptureReadback.h
            CaptureRequest.h
            CaptureService.h
            CaptureTypes.h)

core_add_library(rendering_capture)
