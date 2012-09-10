aux_source_directory(. httpapi_SRCS)
list(REMOVE_ITEM httpapi_SRCS ./XBMCConfiguration.cpp)
add_library(httpapi STATIC ${httpapi_SRCS})
