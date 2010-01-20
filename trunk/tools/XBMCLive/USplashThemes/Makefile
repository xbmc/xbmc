prefix=/usr/local

build:
	make -C pulsating-logo
	make -C spinner-black-silver
	make -C spinner-blue
	make -C spinner-black
	echo $(prefix)
    
install: build
	mkdir -p $(prefix)/lib/usplash
	cp pulsating-logo/xbmc-splash.so $(prefix)/lib/usplash/xbmc-splash-pulsating-logo.so
	cp spinner-black/xbmc-splash.so $(prefix)/lib/usplash/xbmc-splash-spinner-black.so
	cp spinner-blue/xbmc-splash.so $(prefix)/lib/usplash/xbmc-splash-spinner-blue.so
	cp spinner-black-silver/xbmc-splash.so $(prefix)/lib/usplash/xbmc-splash-spinner-black-silver.so
    
all: build

clean:
	find . -regextype posix-extended  -iregex ".*\.o|.*\.so" -exec rm {} \;
