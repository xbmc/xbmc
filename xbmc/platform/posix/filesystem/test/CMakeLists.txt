set(SOURCES)
set(HEADERS)

if(TARGET ${APP_NAME_LC}::SmbClient)
  list(APPEND SOURCES TestSMBFile.cpp)
endif()

if(SOURCES)
  core_add_test_library(platform_posix_filesystem_test)
endif()
