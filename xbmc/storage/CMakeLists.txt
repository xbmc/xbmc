set(SOURCES AutorunMediaJob.cpp
            MediaManager.cpp)

set(HEADERS AutorunMediaJob.h
            IStorageProvider.h
            MediaManager.h)

if(ENABLE_OPTICAL)
  list(APPEND SOURCES cdioSupport.cpp
                      DetectDVDType.cpp)
  list(APPEND HEADERS cdioSupport.h
                      DetectDVDType.h)
endif()

core_add_library(storage)
