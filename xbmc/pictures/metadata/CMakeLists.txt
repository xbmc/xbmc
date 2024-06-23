set(SOURCES ImageMetadataParser.cpp)

set(HEADERS Exif.h
            ImageMetadata.h
            ImageMetadataParser.h
            Iptc.h)

core_add_library(pictures_metadata)
