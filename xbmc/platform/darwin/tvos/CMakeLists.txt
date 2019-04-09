set(SOURCES 
			MainApplication.mm
			MainController.mm
			MainEAGLView.mm
			MainKeyboard.mm
			MainKeyboardView.mm
			TVOSSettingsHandler.mm
			TVOSTopShelf.mm
			tvosShared.m)

set(HEADERS
			MainApplication.h
			MainController.h
			MainEAGLView.h
			MainKeyboard.h
			MainKeyboardView.h
			TVOSSettingsHandler.h
			TVOSTopShelf.h
			tvosShared.h)

core_add_library(platform_tvos)
