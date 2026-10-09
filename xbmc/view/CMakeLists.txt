set(SOURCES GUIViewControl.cpp
            GUIViewState.cpp
            ViewDatabase.cpp
            ViewStateSettings.cpp)

set(HEADERS GUIViewControl.h
            GUIViewState.h
            ViewDatabase.h
            ViewState.h
            ViewStateNames.h
            ViewStateSettings.h)

core_add_library(view)
