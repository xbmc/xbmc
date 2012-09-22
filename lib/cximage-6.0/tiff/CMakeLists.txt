aux_source_directory(. tiff_srcs)
include_directories(../jpeg)
add_library(cx_tiff OBJECT ${tiff_srcs})