set(SOURCES TestBaseRenderBufferPool.cpp
            TestHwRenderingContextEGL.cpp
            TestRenderBufferOpenGLES.cpp
            TestRenderBufferPoolFBO.cpp
            TestRenderGeometryFBO.cpp
            TestRPProcessInfoEGL.cpp
            TestRPRenderManager.cpp
)

if(CORE_SYSTEM_NAME STREQUAL "osx" AND TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES TestHwRenderingContextOSX.mm)
endif()

core_add_test_library(retroplayer_rendering_test)
