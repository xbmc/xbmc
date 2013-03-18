find_all_sources(. client_SRC)
#set(client_SRC ${client_SRC} ../FileSystem/PlexDirectory.cpp)
add_executable(testClient ${client_SRC})
