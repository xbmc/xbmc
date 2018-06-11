set(SOURCES GUIWindowTestPatternGL.cpp
            RenderSystemGL.cpp
            ../MatrixGL.cpp
            GLShader.cpp)

set(HEADERS GUIWindowTestPatternGL.h
            RenderSystemGL.h
            ../MatrixGL.h
            GLShader.h)

core_add_library(rendering_gl)
