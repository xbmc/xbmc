set(SOURCES CPUInfoIos.cpp
            IOSEAGLView.mm
            IOSExternalTouchController.mm
            IOSScreenManager.mm
            IOSSettingsHandler.mm
            XBMCController.mm)

set(HEADERS CPUInfoIos.h
            IOSEAGLView.h
            IOSExternalTouchController.h
            IOSScreenManager.h
            IOSSettingsHandler.h
            XBMCApplication.h
            XBMCController.h)

core_add_library(platform_ios)
