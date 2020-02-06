set(SOURCES CPUInfoFreebsd.cpp
            OptionalsReg.cpp
            ../linux/OptionalsReg.cpp
            ../linux/TimeUtils.cpp
            MemUtils.cpp)

set(HEADERS CPUInfoFreebsd.h
            OptionalsReg.h
            ../linux/OptionalsReg.h
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
