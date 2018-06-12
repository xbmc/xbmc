set(SOURCES TestDirectory.cpp
            TestFile.cpp
            TestFileFactory.cpp
            TestZipFile.cpp
            TestZipManager.cpp)

if(NFS_FOUND)
  list(APPEND SOURCES TestNfsFile.cpp)
endif()

core_add_test_library(filesystem_test)
