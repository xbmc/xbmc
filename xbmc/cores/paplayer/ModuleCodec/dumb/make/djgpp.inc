# This file contains DJGPP-specific definitions. It will be included by the
# main Makefile when you compile with DJGPP.

PLATFORM := djgpp

APOST := \'

# Macro for replacing / with \ where necessary. Usage: $(call FIX,path)
FIX = $(subst /,\,$(subst /*,\\\*,$(1)))

ECHO = @$(COMSPEC) /C ECHO $(1)
# Note: the following two macros only work for single files!
DELETE = $(COMSPEC) /C DEL $(call FIX,$(1))
COPY = $(COMSPEC) /C COPY $(call FIX,$(1)) $(call FIX,$(2))
MKDIR = $(COMSPEC) /C MKDIR $(call FIX,$(1))

EXE_SUFFIX := .exe

LINK_MATH :=
LINK_ALLEGRO := -lalleg

ifndef DJDIR
.PHONY: error
error:
	$(call ECHO,Your DJDIR environment variable is not set!)
	$(call ECHO,Please refer to DJGPP's documentation and install it properly.)
endif

PREFIX := $(DJDIR)
