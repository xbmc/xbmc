set(SOURCES TestAddonsExecuteAddon.cpp
            TestAudioLibrary.cpp
            TestFileOperations.cpp
            TestJSONRPCPermission.cpp
            TestJSONRPCStatus.cpp
            TestPVREpgFields.cpp
            TestSetConfiguration.cpp
            TestSettingsLevel.cpp
            TestVideoLibraryPlaybackState.cpp
            TestVideoLibrarySetSourceContent.cpp)

core_add_test_library(jsonrpc_test)
