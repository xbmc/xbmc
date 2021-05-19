set(SOURCES ConvolutionKernels.cpp)

set(HEADERS ConvolutionKernels.h
            dither.h
            ShaderFormats.h)

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES ConversionMatrix.cpp
                      WinVideoFilter.cpp)
  list(APPEND HEADERS ConversionMatrix.h
                      WinVideoFilter.h)
endif()

if(OPENGL_FOUND)
  list(APPEND SOURCES ConversionMatrix.cpp
                      GLSLOutput.cpp
                      VideoFilterShaderGL.cpp
                      YUV2RGBShaderGL.cpp)
  list(APPEND HEADERS ConversionMatrix.h
                      GLSLOutput.h
                      VideoFilterShaderGL.h
                      YUV2RGBShaderGL.h)
endif()

if(OPENGLES_FOUND AND ("android" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "ios" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "tvos" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "gbm" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "x11" IN_LIST CORE_PLATFORM_NAME_LC OR
                       "wayland" IN_LIST CORE_PLATFORM_NAME_LC))
  list(APPEND SOURCES ConversionMatrix.cpp
                      VideoFilterShaderGLES.cpp
                      YUV2RGBShaderGLES.cpp)
  list(APPEND HEADERS ConversionMatrix.h
                      VideoFilterShaderGLES.h
                      YUV2RGBShaderGLES.h)
endif()

core_add_library(videoshaders)
