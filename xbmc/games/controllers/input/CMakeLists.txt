set(SOURCES InputSink.cpp
            PhysicalFeature.cpp
            PhysicalPort.cpp
            PhysicalTopology.cpp
            PortInput.cpp
            PortManager.cpp
)

set(HEADERS InputSink.h
            PhysicalFeature.h
            PhysicalPort.h
            PhysicalTopology.h
            PortInput.h
            PortManager.h
)

core_add_library(games_controller_input)
