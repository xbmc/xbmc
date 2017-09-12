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

if(OPENGL_FOUND)
  list(APPEND SOURCES VideoFilterShaderGL.cpp
                      GLSLOutput.cpp
                      YUV2RGBShader.cpp)
  list(APPEND HEADERS VideoFilterShaderGL.h
                      GLSLOutput.h
                      YUV2RGBShader.h)
endif()

if(OPENGLES_FOUND AND (CORE_PLATFORM_NAME_LC STREQUAL android OR
                       CORE_PLATFORM_NAME_LC STREQUAL ios OR
                       CORE_PLATFORM_NAME_LC STREQUAL gbm OR
                       CORE_PLATFORM_NAME_LC STREQUAL imx OR
                       CORE_PLATFORM_NAME_LC STREQUAL mir OR
                       CORE_PLATFORM_NAME_LC STREQUAL wayland))
  list(APPEND SOURCES VideoFilterShaderGLES.cpp
                      GLSLOutput.cpp
                      YUV2RGBShader.cpp)
  list(APPEND HEADERS VideoFilterShaderGLES.h
                      GLSLOutput.h
                      YUV2RGBShader.h)
endif()

core_add_library(videoshaders)
