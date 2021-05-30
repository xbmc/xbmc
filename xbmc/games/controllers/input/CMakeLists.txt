set(SOURCES ControllerActivity.cpp
            ControllerState.cpp
            InputSink.cpp
            PhysicalFeature.cpp
            PhysicalTopology.cpp
)

set(HEADERS ControllerActivity.h
            ControllerState.h
            InputSink.h
            PhysicalFeature.h
            PhysicalTopology.h
)

core_add_library(games_controller_input)
