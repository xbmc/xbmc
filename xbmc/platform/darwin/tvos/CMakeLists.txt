configure_file(${CMAKE_CURRENT_SOURCE_DIR}/tvosShared.m.in ${CMAKE_CURRENT_SOURCE_DIR}/tvosShared.m @ONLY)

set(SOURCES
			MainController.mm
			MainEAGLView.mm
			MainKeyboard.mm
			MainKeyboardView.mm
			TVOSSettingsHandler.mm
			TVOSTopShelf.mm
			${CMAKE_CURRENT_SOURCE_DIR}/tvosShared.m)

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
