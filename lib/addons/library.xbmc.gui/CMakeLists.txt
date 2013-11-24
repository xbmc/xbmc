include_directories(
	${root}/xbmc/addons/include
	${root}/xbmc
)

find_all_sources(. addonSRC)
set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PLEX_LINK_WRAPPED}")
add_library(libXBMC_gui-${ARCH} MODULE ${addonSRC})
set_target_properties(libXBMC_gui-${ARCH} PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${root}/addons/library.xbmc.gui)
