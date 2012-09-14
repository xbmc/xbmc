add_subdirectory(MusicDatabaseDirectory)
add_subdirectory(VideoDatabaseDirectory)

include_directories(
	${root}/lib/libUPnP/Platinum/Source/Platinum
	${root}/lib/libUPnP/Platinum/Source/Core
	${root}/lib/libUPnP/Platinum/Source/Devices/MediaServer
	${root}/lib/libUPnP/Neptune/Source/Core
	${root}/lib/libUPnP/Neptune/Source/System/Posix
)

aux_source_directory(. filesystem_SRCS)

list(REMOVE_ITEM filesystem_SRCS ./FileSmb.cpp)
list(REMOVE_ITEM filesystem_SRCS ./NptXbmcFile.cpp)
list(REMOVE_ITEM filesystem_SRCS ./SMBDirectory.cpp)
list(REMOVE_ITEM filesystem_SRCS ./UPnPDirectory.cpp)
list(REMOVE_ITEM filesystem_SRCS ./FileUPnP.cpp)
list(REMOVE_ITEM filesystem_SRCS ./Slingbox.cpp)

add_library(filesystem STATIC ${filesystem_SRCS})
