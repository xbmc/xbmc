set(SOURCES ExifParse.cpp
            GUIDialogPictureInfo.cpp
            GUIViewStatePictures.cpp
            GUIWindowPictures.cpp
            GUIWindowSlideShow.cpp
            IptcParse.cpp
            JpegParse.cpp
            libexif.cpp
            Picture.cpp
            PictureInfoLoader.cpp
            PictureInfoTag.cpp
            PictureScalingAlgorithm.cpp
            PictureThumbLoader.cpp
            SlideShowPicture.cpp)

set(HEADERS GUIDialogPictureInfo.h
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
