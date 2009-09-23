# This file contains MinGW-specific definitions. It will be included by the
# main Makefile when you compile with MinGW.

PLATFORM := mingw

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

ifndef MINGDIR
.PHONY: error
error:
	$(call ECHO,Your MINGDIR environment variable is not set!)
	$(call ECHO,Please set it to point to the directory containing your MinGW installation.)
endif

PREFIX := $(MINGDIR)
