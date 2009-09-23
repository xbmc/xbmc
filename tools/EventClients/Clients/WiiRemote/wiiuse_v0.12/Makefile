#
#	wiiuse Makefile
#

all clean install:
	@$(MAKE) -C src $@
	@$(MAKE) -C example $@
	@$(MAKE) -C example-sdl $@

wiiuse:
	@$(MAKE) -C src

ex:
	@$(MAKE) -C example

sdl-ex:
	@$(MAKE) -C example-sdl

distclean:
	@$(MAKE) -C src $@
	@$(MAKE) -C example $@
	@$(MAKE) -C example-sdl $@
	@find . -type f \( -name "*.save" -or -name "*~" -or -name gmon.out \) -delete &> /dev/null
