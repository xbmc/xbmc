add_subdirectory(MusicDatabaseDirectory)
add_subdirectory(VideoDatabaseDirectory)

find_all_sources(. filesystem_SRCS)

set(removed_files FileSmb.cpp SMBDirectory.cpp Slingbox.cpp FileDAAP.cpp RTVDirectory.cpp FileRTV.cpp DAAPDirectory.cpp APKDirectory.cpp APKFile.cpp SmbFile.cpp BlurayDirectory.cpp)
foreach(r ${removed_files})
	list(REMOVE_ITEM filesystem_SRCS ./${r})
endforeach()

add_sources(${filesystem_SRCS})
