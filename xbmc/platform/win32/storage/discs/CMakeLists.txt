if(ENABLE_OPTICAL)
  set(SOURCES Win32DiscDriveHandler.cpp)

  set(HEADERS Win32DiscDriveHandler.h)

  core_add_library(platform_win32_storage_discs)
endif()
