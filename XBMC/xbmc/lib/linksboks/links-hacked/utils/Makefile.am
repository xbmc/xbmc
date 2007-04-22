# Various utils

bins = generate_font arrow make_included

if FT_AND_GD
bin_PROGRAMS=$(bins) rasterizer
rasterizer_SOURCES = rasterizer.c
else
bin_PROGRAMS=$(bins)
endif

generate_font_SOURCES = generate_font.c

arrow_SOURCES = arrow.c

make_included_SOURCES = make_included.c
