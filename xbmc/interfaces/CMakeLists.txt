add_subdirectory(http-api)
add_subdirectory(info)
add_subdirectory(json-rpc)

aux_source_directory(. interfaces_SRCS)
add_sources(${interfaces_SRCS})
