SET(SRCS
    emu_dummy.cpp
    emu_dx8.cpp
    emu_kernel32.cpp
    exports_kernel32.cpp
    emu_msvcrt.cpp
    exports_msvcrt.cpp
    exports_python_linux.cpp
    wrapper.c)

SET(DIRS util)

SET_PROPERTY(SOURCE exports_python_linux.cpp wrapper.c PROPERTY COMPILE_DEFINITIONS _LINUX _REENTRANT _LARGEFILE64_SOURCE _FILE_OFFSET_BITS=64 __USE_FILE_OFFSET64)

INCLUDE(${CMAKE_SOURCE_DIR}/CMakeLists.common)

