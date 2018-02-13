set(SOURCES RPProcessInfoGbm.cpp)
set(HEADERS RPProcessInfoGbm.h)

if(GBM_HAS_BO_MAP)
  list(APPEND SOURCES RenderBufferGBM.cpp
                      RenderBufferPoolGBM.cpp)
  list(APPEND HEADERS RenderBufferGBM.h
                      RenderBufferPoolGBM.h)
endif()

core_add_library(rp-process-gbm)
