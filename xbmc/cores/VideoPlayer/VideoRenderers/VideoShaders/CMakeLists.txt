set(SOURCES ConvolutionKernels.cpp)

set(HEADERS ConvolutionKernels.h
            dither.h)

if(CORE_SYSTEM_NAME STREQUAL windows)
  list(APPEND SOURCES WinVideoFilter.cpp
                      YUV2RGBShader.cpp)
  list(APPEND HEADERS WinVideoFilter.h
                      YUV2RGBShader.h)
endif()

if(OPENGL_FOUND OR OPENGLES_FOUND)
  list(APPEND SOURCES GLSLOutput.cpp
                      VideoFilterShader.cpp
                      YUV2RGBShader.cpp)
  list(APPEND HEADERS GLSLOutput.h
                      VideoFilterShader.h
                      YUV2RGBShader.h)
endif()

core_add_library(videoshaders)
