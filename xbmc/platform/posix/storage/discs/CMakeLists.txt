if(ENABLE_OPTICAL)
  set(SOURCES DiscDriveHandlerPosix.cpp)

  set(HEADERS DiscDriveHandlerPosix.h)

  core_add_library(platform_posix_storage_discs)
endif()
