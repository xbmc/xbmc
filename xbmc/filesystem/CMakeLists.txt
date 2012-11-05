add_subdirectory(MusicDatabaseDirectory)
add_subdirectory(VideoDatabaseDirectory)

include_directories(
	${root}/lib/libUPnP/Platinum/Source/Platinum
	${root}/lib/libUPnP/Platinum/Source/Core
	${root}/lib/libUPnP/Platinum/Source/Devices/MediaServer
	${root}/lib/libUPnP/Neptune/Source/Core
	${root}/lib/libUPnP/Neptune/Source/System/Posix
)

find_all_sources(. filesystem_SRCS)

set(removed_files FileSmb.cpp NptXbmcFile.cpp SMBDirectory.cpp UPnPDirectory.cpp FileUPnP.cpp Slingbox.cpp FileDAAP.cpp RTVDirectory.cpp FileRTV.cpp DAAPDirectory.cpp APKDirectory.cpp APKFile.cpp SmbFile.cpp)
foreach(r ${removed_files})
	list(REMOVE_ITEM filesystem_SRCS ./${r})
endforeach()

add_sources(${filesystem_SRCS})
