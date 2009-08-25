ARCH=i486-linux
INCLUDES=-I. -I../../addons/include -I../librefmem
SO=AddonLib-$(ARCH).so
ARCHIVE=$(SO:.so=.a)
SYSDIR=../../../system
SLIB=$(SYSDIR)/$(SO)
DIRS=../librefmem
SLIBS=../librefmem/refmem.a

.PHONY: compile

$(SLIB): $(SLIBS)
	$(CXX) -shared -o $(SLIB) $(SLIBS)

$(SLIBS): compile

refmem.a:
	$(MAKE) -C ../librefmem

include ../../../Makefile.include
