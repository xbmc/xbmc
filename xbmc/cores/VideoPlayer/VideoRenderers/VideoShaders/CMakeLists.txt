set(SOURCES ConvolutionKernels.cpp)

set(HEADERS ConvolutionKernels.h
            dither.h
            ShaderFormats.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES WinVideoFilter.cpp
                      YUV2RGBShader.cpp)
  list(APPEND HEADERS WinVideoFilter.h
                      YUV2RGBShader.h)
endif()

if(OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES GLSLOutput.cpp
                      YUV2RGBShader.cpp)
  list(APPEND HEADERS GLSLOutput.h
                      YUV2RGBShader.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES VideoFilterShaderGL.cpp)
  list(APPEND HEADERS VideoFilterShaderGL.h)
endif()

core_add_library(videoshaders)
