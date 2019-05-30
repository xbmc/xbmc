set(SOURCES MemUtils.cpp
            ../linux/OptionalsReg.cpp
            ../linux/TimeUtils.cpp)

set(HEADERS ../linux/OptionalsReg.h
            ../linux/PlatformConstants.h
            ../linux/TimeUtils.h)

if(ALSA_FOUND)
  list(APPEND SOURCES ../linux/FDEventMonitor.cpp)
  list(APPEND HEADERS ../linux/FDEventMonitor.h)
endif()

if(DBUS_FOUND)
  list(APPEND SOURCES ../linux/DBusMessage.cpp
                      ../linux/DBusReserve.cpp
                      ../linux/DBusUtil.cpp)
  list(APPEND HEADERS ../linux/DBusMessage.h
                      ../linux/DBusReserve.h
                      ../linux/DBusUtil.h)
endif()

core_add_library(freebsdsupport)
