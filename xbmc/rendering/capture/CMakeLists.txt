set(SOURCES CaptureHandle.cpp
            CaptureMetadata.cpp
            CaptureService.cpp)

set(HEADERS CaptureHandle.h
            CaptureMetadata.h
            CaptureRequest.h
            CaptureService.h
            CaptureTypes.h)

core_add_library(rendering_capture)
