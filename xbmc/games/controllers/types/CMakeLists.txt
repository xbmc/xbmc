set(SOURCES ControllerGrid.cpp
            ControllerHub.cpp
            ControllerNode.cpp
            PortNode.cpp
)

set(HEADERS ControllerGrid.h
            ControllerHub.h
            ControllerNode.h
            ControllerTree.h
            PortNode.h
)

core_add_library(games_controller_types)
