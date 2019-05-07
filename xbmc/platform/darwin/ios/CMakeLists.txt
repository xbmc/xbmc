set(SOURCES CPUInfoIos.cpp
            IOSEAGLView.mm
            IOSExternalTouchController.mm
            IOSScreenManager.mm
            XBMCController.mm)

set(HEADERS CPUInfoIos.h
            IOSEAGLView.h
            IOSExternalTouchController.h
            IOSScreenManager.h
            XBMCApplication.h
            XBMCController.h)

core_add_library(platform_ios)
