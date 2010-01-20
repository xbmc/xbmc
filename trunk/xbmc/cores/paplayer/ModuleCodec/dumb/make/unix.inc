# This file contains definitions suitable for Unix-compatible systems. It will
# be included by the main Makefile when you compile on such a system.

PLATFORM := unix

APOST := \'

# Macro that on DOS and Windows would replace / with \. Usage: $(call FIX,path)
FIX = $(1)

ECHO = @echo $(1)
DELETE = rm -f $(1)
COPY = cp $(1) $(2)
MKDIR = mkdir $(1)

EXE_SUFFIX :=

LINK_MATH := -lm
LINK_ALLEGRO := `allegro-config --libs`

# PREFIX is set by config.sh.
