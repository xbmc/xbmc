find_all_sources(lib enca_SRCS)
include_directories(enca libenca_win32 ${CMAKE_CURRENT_SOURCE_DIR}
add_definitions(-D_LIB -DHAVE_CONFIG_H -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SECURE_SCL=0 -D_HAS_ITERATOR_DEBUGGING=0)
add_library(enca STATIC ${enca_SRCS})
