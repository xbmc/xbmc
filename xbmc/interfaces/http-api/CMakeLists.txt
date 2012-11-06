find_all_sources(. httpapi_SRCS)
list(REMOVE_ITEM httpapi_SRCS ./XBMCConfiguration.cpp)
add_sources(${httpapi_SRCS})
