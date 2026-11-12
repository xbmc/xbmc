set(SOURCES TestActiveAEBuffer.cpp
            TestActiveAEEngineStats.cpp
            TestActiveAEFilter.cpp
            TestActiveAEResampleFFMPEG.cpp
            TestActiveAEStream.cpp
            TestActiveAESync.cpp)

core_add_test_library(audioengine_activeae_test)
