aux_source_directory(. httpapi_SRCS)
list(REMOVE_ITEM httpapi_SRCS ./XBMCConfiguration.cpp)
add_sources(${httpapi_SRCS})
