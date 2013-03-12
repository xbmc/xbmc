add_definitions(-D_USRDLL -DLIBASS_WIN32_EXPORTS -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE)
find_all_sources(libass ass_SRCS)
list(APPEND ass_SRCS xbmc/libass_win32/libass.def)
include_directories(libass xbmc/libass_win32
                           ${root}/lib/freetype/include
                           ${root}/lib/freetype
                           ${dependdir}/include
                           ${root}/lib/enca/lib
                           ${root}/lib/libiconv/include)
add_library(libass SHARED ${ass_SRCS})
target_link_libraries(libass enca ${CONFIG_LIBRARY_LIBICONV} ${CONFIG_LIBRARY_FREETYPE246MT} ${CONFIG_LIBRARY_LIBFRIBIDI} ${CONFIG_LIBRARY_FONTCONFIG})
install(TARGETS libass RUNTIME DESTINATION ${RESOURCEPATH}/system/players/dvdplayer LIBRARY DESTINATION ${RESOURCEPATH}/system/players/dvdplayer COMPONENT RUNTIME)
