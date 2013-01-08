find_all_sources(. tiff_srcs)
include_directories(../jpeg)
add_library(cx_tiff STATIC ${tiff_srcs})
