set(SOURCES GUIDialogPictureInfo.cpp
            GUIViewStatePictures.cpp
            GUIWindowPictures.cpp
            GUIWindowSlideShow.cpp
            Picture.cpp
            PictureInfoLoader.cpp
            PictureInfoTag.cpp
            PictureScalingAlgorithm.cpp
            PictureThumbLoader.cpp
            SlideShowPicture.cpp)

set(HEADERS DllLibExif.h
            GUIDialogPictureInfo.h
            GUIViewStatePictures.h
            GUIWindowPictures.h
            GUIWindowSlideShow.h
            Picture.h
            PictureInfoLoader.h
            PictureInfoTag.h
            PictureScalingAlgorithm.h
            PictureThumbLoader.h
            SlideShowPicture.h)

core_add_library(pictures)
