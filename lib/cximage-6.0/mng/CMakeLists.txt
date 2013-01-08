find_all_sources(. mng_srcs)
add_definitions(-DMNG_SUPPORT_DISPLAY -DMNG_SUPPORT_READ -DMNG_SUPPORT_WRITE -DMNG_ACCESS_CHUNKS -DMNG_ACCESS_CHUNKS)
add_library(cx_mng OBJECT ${mng_srcs})
