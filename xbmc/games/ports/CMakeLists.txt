set(SOURCES InputSink.cpp
            Port.cpp
            PortInput.cpp
            PortManager.cpp
            PortMapper.cpp)

set(HEADERS InputSink.h
            Port.h
            PortInput.h
            PortManager.h
            PortMapper.h
            PortTypes.h)

core_add_library(gameports)
