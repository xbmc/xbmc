set(SOURCES ExifParse.cpp
            IptcParse.cpp
            JpegParse.cpp
            libexif.cpp)

set(CMAKE_POSITION_INDEPENDENT_CODE 1)

core_add_library(exif NO_MAIN_DEPENDS)
target_compile_definitions(exif PRIVATE _LINUX _DLL)
