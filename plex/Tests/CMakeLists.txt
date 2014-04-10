if(ENABLE_TESTING)
	find_all_sources(. SRCS)
	add_library(PlexTestingUtils OBJECT ${SRCS})
endif(ENABLE_TESTING)
