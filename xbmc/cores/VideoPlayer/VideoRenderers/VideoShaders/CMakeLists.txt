set(SOURCES ConvolutionKernels.cpp
            YUVMatrix.cpp)

set(HEADERS ConvolutionKernels.h
            YUVMatrix.h
            dither.h
            ShaderFormats.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES WinVideoFilter.cpp)
  list(APPEND HEADERS WinVideoFilter.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES GLSLOutput.cpp
                      VideoFilterShaderGL.cpp
                      YUV2RGBShaderGL.cpp)
  list(APPEND HEADERS GLSLOutput.h
                      VideoFilterShaderGL.h
                      YUV2RGBShaderGL.h)
endif()

if(OPENGLES_FOUND AND (CORE_PLATFORM_NAME_LC STREQUAL android OR
                       CORE_PLATFORM_NAME_LC STREQUAL ios OR
                       CORE_PLATFORM_NAME_LC STREQUAL aml OR
                       CORE_PLATFORM_NAME_LC STREQUAL gbm OR
                       CORE_PLATFORM_NAME_LC STREQUAL imx OR
                       CORE_PLATFORM_NAME_LC STREQUAL mir OR
                       CORE_PLATFORM_NAME_LC STREQUAL wayland))
  list(APPEND SOURCES GLSLOutput.cpp
                      VideoFilterShaderGLES.cpp
                      YUV2RGBShaderGLES.cpp)
  list(APPEND HEADERS GLSLOutput.h
                      VideoFilterShaderGLES.h
                      YUV2RGBShaderGLES.h)
endif()

core_add_library(videoshaders)
