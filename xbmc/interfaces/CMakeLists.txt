add_subdirectory(http-api)
add_subdirectory(info)
add_subdirectory(json-rpc)

find_all_sources(. interfaces_SRCS)
add_sources(${interfaces_SRCS})
