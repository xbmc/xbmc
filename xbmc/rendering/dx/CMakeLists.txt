set(SOURCES DeviceResources.cpp
            RenderSystemDX.cpp)

set(HEADERS DeviceResources.h
            DirectXHelper.h
            RenderContext.h
            RenderSystemDX.h)

core_add_library(rendering_dx)
