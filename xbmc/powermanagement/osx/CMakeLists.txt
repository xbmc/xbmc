set(SOURCES CocoaPowerSyscall.cpp)

set(HEADERS CocoaPowerSyscall.h)

core_add_library(powermanagement_osx)
add_dependencies(powermanagement_osx libcpluff)
