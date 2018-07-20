include ../../Makefile.include
DEPS= ../../Makefile.include Makefile ios11_entitlements.xml

IOS11_ENT=$(PREFIX)/share/ios11_entitlements.xml

all: $(IOS11_ENT)

$(IOS11_ENT):
	mkdir -p $(PREFIX)/share
	cp ios11_entitlements.xml $(IOS11_ENT)


clean:
distclean::
	rm -f $(IOS11_ENT)
