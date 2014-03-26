find_all_sources(. client_SRC)
find_all_sources(MyPlex myPlex_SRC)

add_sources(${client_SRC} ${myPlex_SRC})

if(ENABLE_TESTING)
	add_subdirectory(Tests)
endif(ENABLE_TESTING)