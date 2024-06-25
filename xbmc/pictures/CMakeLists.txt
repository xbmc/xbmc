set(SOURCES GUIDialogPictureInfo.cpp
            GUIViewStatePictures.cpp
            GUIWindowPictures.cpp
            GUIWindowSlideShow.cpp
            Picture.cpp
            PictureFolderImageFileLoader.cpp
            PictureInfoLoader.cpp
            PictureInfoTag.cpp
            PictureScalingAlgorithm.cpp
            PictureThumbLoader.cpp
            SlideShowDelegator.cpp
            SlideShowPicture.cpp)

set(HEADERS interfaces/ISlideShowDelegate.h
            GUIDialogPictureInfo.h
            GUIViewStatePictures.h
            GUIWindowPictures.h
            GUIWindowSlideShow.h
            Picture.h
            PictureFolderImageFileLoader.h
            PictureInfoLoader.h
            PictureInfoTag.h
            PictureScalingAlgorithm.h
            PictureThumbLoader.h
            SlideShowDelegator.h
            SlideShowPicture.h)

if(TARGET ${APP_NAME_LC}::OpenGl)
  list(APPEND SOURCES SlideShowPictureGL.cpp)
  list(APPEND HEADERS SlideShowPictureGL.h)
endif()

if(TARGET ${APP_NAME_LC}::OpenGLES)
  list(APPEND SOURCES SlideShowPictureGLES.cpp)
  list(APPEND HEADERS SlideShowPictureGLES.h)
endif()

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES SlideShowPictureDX.cpp)
  list(APPEND HEADERS SlideShowPictureDX.h)
endif()

core_add_library(pictures)
