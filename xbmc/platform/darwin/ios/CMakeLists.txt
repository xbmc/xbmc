set(SOURCES IOSEAGLView.mm
            IOSExternalTouchController.mm
            IOSScreenManager.mm
            XBMCController.mm)

set(HEADERS IOSEAGLView.h
            IOSExternalTouchController.h
            IOSScreenManager.h
            XBMCApplication.h
            XBMCController.h
            XBMCDebugHelpers.h)

core_add_library(platform_ios)
