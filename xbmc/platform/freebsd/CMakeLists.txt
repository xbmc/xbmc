set(SOURCES ../linux/AppParamParserLinux.cpp
            CPUInfoFreebsd.cpp
            OptionalsReg.cpp
            ../linux/OptionalsReg.cpp
            ../linux/TimeUtils.cpp
            MemUtils.cpp
            PlatformFreebsd.cpp)

set(HEADERS ../linux/AppParamParserLinux.cpp
            CPUInfoFreebsd.h
            OptionalsReg.h
            ../linux/OptionalsReg.h
            ../linux/TimeUtils.h
            PlatformFreebsd.h)

if(ALSA_FOUND)
  list(APPEND SOURCES ../linux/FDEventMonitor.cpp)
  list(APPEND HEADERS ../linux/FDEventMonitor.h)
endif()

if(DBUS_FOUND)
  list(APPEND SOURCES ../linux/DBusMessage.cpp
                      ../linux/DBusUtil.cpp)
  list(APPEND HEADERS ../linux/DBusMessage.h
                      ../linux/DBusUtil.h)
endif()

core_add_library(freebsdsupport)
