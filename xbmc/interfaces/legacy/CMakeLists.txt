find_all_sources(. legacy_src)
add_definitions(-DSWIGRUNTIME_DEBUG -DSTATIC_LINKED)
add_library(addons_legacy STATIC ${legacy_src})