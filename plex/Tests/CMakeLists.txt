if(ENABLE_TESTING)
	add_library(PlexTestingUtils OBJECT PlexTestUtils.cpp)
endif(ENABLE_TESTING)

plex_add_testcase(PlexGUIInfoManagerTests.cpp)
