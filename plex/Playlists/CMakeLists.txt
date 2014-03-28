find_all_sources(. PLAYLIST_SRCS)
add_sources(${PLAYLIST_SRCS})

if(ENABLE_TESTING)
	add_subdirectory(Tests)
endif(ENABLE_TESTING)