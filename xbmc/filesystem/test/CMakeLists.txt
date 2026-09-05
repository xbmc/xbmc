set(SOURCES TestAudioBookFileDirectory.cpp
            TestDirectory.cpp
            TestDirectoryCache.cpp
            TestDiscDirectoryHelper.cpp
            TestFile.cpp
            TestFileFactory.cpp
            TestZipFile.cpp
            TestZipManager.cpp)

if(TARGET ${APP_NAME_LC}::Bluray)
  list(APPEND SOURCES TestBlurayDirectory.cpp
                      TestStreamParser.cpp)
endif()

if(TARGET ${APP_NAME_LC}::MicroHttpd)
  list(APPEND SOURCES TestHTTPDirectory.cpp)
endif()

if(TARGET ${APP_NAME_LC}::NFS)
  list(APPEND SOURCES TestNfsFile.cpp)
endif()

# the tested implementation is the posix one, windows has its own
if(TARGET ${APP_NAME_LC}::SmbClient AND NOT CORE_SYSTEM_NAME STREQUAL windows AND
   NOT CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES TestSMBFile.cpp)
endif()

core_add_test_library(filesystem_test)
