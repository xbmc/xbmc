set(SOURCES ExifParse.cpp
            IptcParse.cpp
            JpegParse.cpp
            libexif.cpp)

set(CMAKE_POSITION_INDEPENDENT_CODE 1)

add_definitions(-D_LINUX -D_DLL)

core_add_library(exif NO_MAIN_DEPENDS)
