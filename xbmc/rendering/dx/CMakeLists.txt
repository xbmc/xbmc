set(SOURCES DeviceResources.cpp
            GUIWindowTestPatternDX.cpp
            RenderSystemDX.cpp)

set(HEADERS DeviceResources.h
            DirectXHelper.h
            GUIWindowTestPatternDX.h
            RenderContext.h
            RenderSystemDX.h)

core_add_library(rendering_dx)
