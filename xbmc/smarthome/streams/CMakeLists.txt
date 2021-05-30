set(SOURCES SmartHomeStreamManager.cpp
            SmartHomeStreamSwFramebuffer.cpp
)

set(HEADERS SmartHomeStreamManager.h
            SmartHomeStreamSwFramebuffer.h
            ISmartHomeStream.h
)

core_add_library(smarthome_streams)
