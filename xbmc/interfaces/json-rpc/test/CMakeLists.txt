set(SOURCES TestAddonsExecuteAddon.cpp
            TestAudioLibrary.cpp
            TestFileItemHandler.cpp
            TestFileOperations.cpp
            TestJSONRPCPermission.cpp
            TestJSONRPCStatus.cpp
            TestJSONServiceDescription.cpp
            TestPVREpgFields.cpp
            TestPlayerOpenDirectory.cpp
            TestSetConfiguration.cpp
            TestSettingsLevel.cpp
            TestVideoLibraryPlaybackState.cpp
            TestVideoLibrarySetSourceContent.cpp)

core_add_test_library(jsonrpc_test)
