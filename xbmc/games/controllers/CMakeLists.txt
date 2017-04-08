set(SOURCES Controller.cpp
            ControllerFeature.cpp
            ControllerLayout.cpp
            ControllerManager.cpp
            ControllerTranslator.cpp)

set(HEADERS Controller.h
            ControllerDefinitions.h
            ControllerFeature.h
            ControllerLayout.h
            ControllerManager.h
            ControllerTranslator.h
            ControllerTypes.h)

core_add_library(games_controller)
