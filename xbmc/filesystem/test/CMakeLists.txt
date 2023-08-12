set(SOURCES TestDirectory.cpp
            TestFile.cpp
            TestFileFactory.cpp
            TestZipFile.cpp
            TestZipManager.cpp)

if(MICROHTTPD_FOUND)
  list(APPEND SOURCES TestHTTPDirectory.cpp)
endif()

if(TARGET libnfs::nfs)
  list(APPEND SOURCES TestNfsFile.cpp)
endif()

core_add_test_library(filesystem_test)
