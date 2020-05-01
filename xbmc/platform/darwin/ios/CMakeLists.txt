set(SOURCES IOSEAGLView.mm
            IOSExternalTouchController.mm
            IOSKeyboardView.mm
            IOSScreenManager.mm
            XBMCController.mm)

set(HEADERS IOSEAGLView.h
            IOSExternalTouchController.h
            IOSKeyboardView.h
            IOSScreenManager.h
            XBMCApplication.h
            XBMCController.h)

set(UNITY_EXCLUDES ${SOURCES}
                   XBMCApplication.mm)

core_add_library(platform_ios)
