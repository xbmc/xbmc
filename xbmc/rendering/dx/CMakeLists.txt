set(SOURCES DeviceResources.cpp
            DirectXHelper.cpp
            RenderSystemDX.cpp
            ScreenshotSurfaceWindows.cpp)

set(HEADERS DeviceResources.h
            DirectXHelper.h
            RenderContext.h
            RenderSystemDX.h
            ScreenshotSurfaceWindows.h)

core_add_library(rendering_dx)
