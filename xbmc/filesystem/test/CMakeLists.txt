set(SOURCES TestDirectory.cpp 
            TestFile.cpp
            TestFileFactory.cpp
            TestRarFile.cpp
            TestZipFile.cpp
            TestZipManager.cpp)

core_add_test_library(filesystem_test)
