set(SOURCES TestAutosaveCapture.cpp
            TestDiscStateHistory.cpp
            TestGameLoop.cpp
            TestMemoryStream.cpp
            TestRetroPlayerPause.cpp
            TestSavestateCapture.cpp
            TestSavestateWorker.cpp)

core_add_test_library(retroplayer_playback_test)
