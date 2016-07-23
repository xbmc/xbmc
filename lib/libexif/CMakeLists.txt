set(SOURCES ExifParse.cpp
            IptcParse.cpp
            JpegParse.cpp
            libexif.cpp)

set(HEADERS ExifParse.h
            IptcParse.h
            JpegParse.h
            libexif.h)

core_add_shared_library(exif WRAPPED)
target_compile_definitions(exif PRIVATE _LINUX _DLL)
set_target_properties(exif PROPERTIES FOLDER lib)
source_group_by_folder(exif)
