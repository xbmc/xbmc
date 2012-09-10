add_subdirectory(MusicDatabaseDirectory)
add_subdirectory(VideoDatabaseDirectory)

aux_source_directory(. filesystem_SRCS)

list(REMOVE_ITEM filesystem_SRCS ./FileSmb.cpp)
list(REMOVE_ITEM filesystem_SRCS ./NptXbmcFile.cpp)
list(REMOVE_ITEM filesystem_SRCS ./SMBDirectory.cpp)
list(REMOVE_ITEM filesystem_SRCS ./UPnPDirectory.cpp)
list(REMOVE_ITEM filesystem_SRCS ./FileUPnP.cpp)
list(REMOVE_ITEM filesystem_SRCS ./Slingbox.cpp)

add_library(filesystem STATIC ${filesystem_SRCS})
