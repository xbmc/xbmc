
GOOM_SRC="
goom2k4-0/src/config_param.c \
goom2k4-0/src/convolve_fx.c \
goom2k4-0/src/cpu_info.c \
goom2k4-0/src/drawmethods.c \
goom2k4-0/src/filters.c \
goom2k4-0/src/flying_stars_fx.c \
goom2k4-0/src/gfontlib.c \
goom2k4-0/src/gfontrle.c \
goom2k4-0/src/goom_core.c \
goom2k4-0/src/goom_tools.c \
goom2k4-0/src/goomsl.c \
goom2k4-0/src/goomsl_hash.c \
goom2k4-0/src/goomsl_heap.c \
goom2k4-0/src/goomsl_lex.c \
goom2k4-0/src/goomsl_yacc.c \
goom2k4-0/src/graphic.c \
goom2k4-0/src/ifs.c \
goom2k4-0/src/jitc_test.c \
goom2k4-0/src/jitc_x86.c \
goom2k4-0/src/lines.c \
goom2k4-0/src/mathtools.c \
goom2k4-0/src/mmx.c \
goom2k4-0/src/plugin_info.c \
goom2k4-0/src/sound_tester.c \
goom2k4-0/src/surf3d.c \
goom2k4-0/src/tentacle3d.c \
goom2k4-0/src/v3d.c \
goom2k4-0/src/xmmx.c \
"

gcc -c -O3 -g -D_WIN32PC -DHAVE_MMX -D_MINGW -Igoom2k4-0/src/ -I../../../lib/libSDL-WIN32/include/ -I../../../visualisations/ ${GOOM_SRC} Main.cpp

gcc -g -s -shared -o ../../../visualisations/goom_win32.vis *.o -lopengl32 -lstdc++
rm *.o
