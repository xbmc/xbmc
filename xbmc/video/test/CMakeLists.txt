set(SOURCES TestBookmark.cpp
            TestFilenameAttributes.cpp
            TestGUIWindowVideoBase.cpp
            TestPlayerControllerActions.cpp
            TestSetInfoTag.cpp
            TestStacks.cpp
            TestVideoDatabase.cpp
            TestVideoDbUrl.cpp
            TestVideoFileItemClassify.cpp
            TestVideoInfoScanner.cpp
            TestVideoInfoScannerArt.cpp
            TestVideoInfoTag.cpp
            TestVideoStreamSelect.cpp
            TestVideoTagLoaderFFmpeg.cpp
            TestVideoTagLoaderNFO.cpp
            TestVideoThumbLoader.cpp
            TestVideoUtils.cpp)

core_add_test_library(video_test)
