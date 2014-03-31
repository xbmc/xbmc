find_all_sources(. client_SRC)
find_all_sources(MyPlex myPlex_SRC)

add_sources(${client_SRC} ${myPlex_SRC})

add_subdirectory(Tests)