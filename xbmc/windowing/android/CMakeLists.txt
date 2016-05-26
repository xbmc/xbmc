set(SOURCES WinEventsAndroid.cpp)

set(HEADERS WinEventsAndroid.h)

core_add_library(windowing_android)
add_dependencies(windowing_android libcpluff)
