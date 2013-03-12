if(NOT WIN32)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fno-common")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fno-common")
  add_definitions(-D_DLL -DXBMC)
endif()

include_directories(goom2k4-0/src)

set(goom_src
  Main.cpp
  goom2k4-0/src/goomsl_yacc.c
  goom2k4-0/src/goomsl_lex.c
  goom2k4-0/src/goomsl.c
  goom2k4-0/src/goomsl_hash.c
  goom2k4-0/src/goomsl_heap.c 
  goom2k4-0/src/goom_tools.c
  goom2k4-0/src/mmx.c
  goom2k4-0/src/xmmx.c
  goom2k4-0/src/config_param.c
  goom2k4-0/src/convolve_fx.c
  goom2k4-0/src/filters.c
  goom2k4-0/src/flying_stars_fx.c
  goom2k4-0/src/gfontlib.c
  goom2k4-0/src/gfontrle.c
  goom2k4-0/src/goom_core.c
  goom2k4-0/src/graphic.c
  goom2k4-0/src/ifs.c
  goom2k4-0/src/lines.c
  goom2k4-0/src/mathtools.c
  goom2k4-0/src/sound_tester.c
  goom2k4-0/src/surf3d.c
  goom2k4-0/src/tentacle3d.c
  goom2k4-0/src/plugin_info.c
  goom2k4-0/src/v3d.c
  goom2k4-0/src/drawmethods.c
  goom2k4-0/src/cpu_info.c
)

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PLEX_LINK_WRAPPED}")
add_library(Goom MODULE ${goom_src})
target_link_libraries(Goom ${CONFIG_LIBRARY_PNG} ${PLEX_LINK_WHOLEARCHIVE} wrapper ${PLEX_LINK_NOWHOLEARCHIVE})

install(DIRECTORY visualization.goom DESTINATION ${RESOURCEPATH}/addons COMPONENT RUNTIME)
install(TARGETS Goom DESTINATION ${RESOURCEPATH}/addons/visualization.goom COMPONENT RUNTIME)
