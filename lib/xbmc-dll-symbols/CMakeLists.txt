include_directories(${CMAKE_BINARY_DIR}/xbmc)
find_all_sources(. symbols_SRCS)
add_library(lib_dllsymbols STATIC ${symbols_SRCS})
