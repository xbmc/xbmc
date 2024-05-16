set(SOURCES TestDirectory.cpp
            TestFile.cpp
            TestFileFactory.cpp
            TestZipFile.cpp
            TestZipManager.cpp)

if(TARGET ${APP_NAME_LC}::MicroHttpd)
  list(APPEND SOURCES TestHTTPDirectory.cpp)
endif()

if(TARGET ${APP_NAME_LC}::NFS)
  list(APPEND SOURCES TestNfsFile.cpp)
endif()

core_add_test_library(filesystem_test)
