set(SOURCES CaptureBlit.cpp
            CaptureHandle.cpp
            CaptureMetadata.cpp
            CaptureService.cpp)

set(HEADERS CaptureBlit.h
            CaptureHandle.h
            CaptureMetadata.h
            CaptureRequest.h
            CaptureService.h
            CaptureTypes.h)

core_add_library(rendering_capture)
