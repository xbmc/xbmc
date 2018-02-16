set(SOURCES Controller.cpp
            ControllerFeature.cpp
            ControllerLayout.cpp
            ControllerManager.cpp
            ControllerTopology.cpp
            ControllerTranslator.cpp)

set(HEADERS Controller.h
            ControllerDefinitions.h
            ControllerFeature.h
            ControllerIDs.h
            ControllerLayout.h
            ControllerManager.h
            ControllerTopology.h
            ControllerTranslator.h
            ControllerTypes.h)

core_add_library(games_controller)
