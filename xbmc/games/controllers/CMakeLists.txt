set(SOURCES Controller.cpp
            ControllerFeature.cpp
            ControllerLayout.cpp
            ControllerTranslator.cpp)

set(HEADERS Controller.h
            ControllerDefinitions.h
            ControllerFeature.h
            ControllerLayout.h
            ControllerTranslator.h
            ControllerTypes.h)

core_add_library(games_controller)
