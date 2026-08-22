set(SOURCES TestAudioBookFileDirectory.cpp
            TestDirectory.cpp
            TestDirectoryCache.cpp
            TestDiscDirectoryHelper.cpp
            TestFile.cpp
            TestFileCache.cpp
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

if(TARGET ${APP_NAME_LC}::SmbClient)
  list(APPEND SOURCES TestSMBFile.cpp)
endif()

core_add_test_library(filesystem_test)
