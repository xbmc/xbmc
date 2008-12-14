# 
#	Makefile -- Top level Makefile for AppWeb
#
#	Copyright (c) Mbedthis Software LLC, 2003-2007. All Rights Reserved.
#
#
#	Standard Make targets supported are:
#	
#		make 						# Does a "make compile"
#		make clean					# Removes generated objects
#		make compile				# Compiles the source
#		make depend					# Generates the make dependencies
#		make test 					# Runs unit tests
#		make leakTest 				# Runs memory leak tests
#		make loadTest 				# Runs load tests
#		make benchmark 				# Runs benchmarks
#		make package				# Creates an installable package
#		make startService			# Starts an installed instance of the app
#		make stopService			# Stops an installed instance of the app
#
#	Additional targets for this makefile:
#
#		make newbuild				# Increment the build number and rebuild
#
#	Installation targets. Use "make DESTDIR=myDir" to do a custom local
#		install:
#
#		make install				# Call install-binary
#		make install-release		# Install release files (README.TXT etc)
#		make install-binary			# Install binary files
#		make install-dev			# Install development libraries and headers
#		make install-doc			# Install documentation
#		make install-samples		# Install samples
#		make install-src			# Install source code
#		make install-all			# Install everything except source code
#		make install-package		# Install a complete installation package
#
#	To remove, use make uninstall-ITEM, where ITEM is a component above.
#

include		build/make/Makefile.top

ifeq ($(shell [ -f buildConfig.make ] && echo found),found)
uclinuxCheck: 
	@if [ "$(UCLINUX_BUILD_USER)" = 1 ] ; \
	then \
		rm -f conf/buildConfig.defaults ; \
		BLD_PRODUCT=appweb ; \
		echo "    ln -s $$BLD_PRODUCT/uclinux.defaults conf/buildConfig.defaults" ;\
		ln -s $$BLD_PRODUCT/uclinux.defaults conf/buildConfig.defaults ; \
		if [ ! -f conf/buildConfig.cache -o ../../.config -nt buildConfig.make ] ; \
		then \
			if [ "$$CONFIG_USER_APPWEB_DYNAMIC" = "y" ] ; \
			then \
				SW="$$SW --enable-modules \
					--with-auth=loadable \
					--with-cgi=loadable \
					--with-copy=loadable \
					--with-esp=loadable \
					--with-copy=loadable" ; \
				moduleType=loadable ; \
			else \
				SW="$$SW --disable-modules" ; \
				moduleType=builtin ; \
			fi ; \
			if [ "$$CONFIG_USER_APPWEB_MULTITHREAD" = "y" ] ; \
			then \
				SW="$$SW --enable-multi-thread" ; \
			else SW="$$SW --disable-multi-thread" ; \
			fi ; \
			if [ "$$CONFIG_USER_APPWEB_SSL" = "y" ] ; \
			then \
				SW="$$SW --with-openssl=$$moduleType" ; \
				SW="$$SW --with-openssl-dir=../../lib/libssl" ; \
			elif [ "$$CONFIG_USER_APPWEB_MATRIXSSL" = "y" ] ; \
			then \
				SW="$$SW --with-matrixssl=$$moduleType" ; \
				SW="$$SW --with-matrixssl-dir=../../lib/matrixssl" ; \
				SW="$$SW --with-matrixssl-iflags=-../../lib/libmatrixssl" ; \
			else SW="$$SW --without-ssl" ; \
			fi ; \
			if [ "$$CONFIG_USER_APPWEB_CGI" = "y" ] ; \
			then SW="$$SW --with-cgi=$$moduleType" ; \
			else SW="$$SW --without-cgi" ; \
			fi ; \
			echo "    ./configure $$SW " ; \
			./configure $$SW; \
			echo "  $(MAKE) -S $(MAKEF)" ; \
			$(MAKE) -S $(MAKEF) ; \
		fi ; \
	else \
		echo "Must run configure first" ; \
		exit 2 ; \
	fi
endif

startService:

stopService:

#
#	!!!!! ONLY IMPORT Build Tools !!!!!!
#
#	import:
#	@bash tools/import.ksh -v ../../build/trunk/build/export/export.all


#
#	Convenient configure targets
#
vx5:
	unset WIND_HOME WIND_BASE ; \
	SEARCH_PATH=/tornado ./configure --product=appweb --defaults=standard \
	--type=DEBUG --host=i386-wrs-vxworks \
	--enable-ranges --enable-multithread --enable-assert --enable-squeeze \
	--disable-floating-point --disable-legacy-api --disable-access-log \
	--disable-shared --disable-samples --disable-run-as-service \
	--without-ssl --without-matrixssl --without-php5 --without-openssl \
	--without-cgi --without-gacompat --disable-modules

vx6:
	unset WIND_HOME WIND_BASE ; \
	./configure --product=appweb --defaults=standard --type=DEBUG --host=pentium-wrs-vxworks \
	--enable-ranges --enable-multithread --enable-assert --enable-squeeze \
	--disable-floating-point --disable-legacy-api --disable-access-log \
	--disable-shared --disable-samples --disable-run-as-service \
	--without-ssl --without-matrixssl --without-php5 --without-openssl \
	--without-cgi --without-gacompat --disable-modules


#
#	Samples for cross compilation
#	
vx5env:
	ARCH=386 ; \
	WIND_HOME=c:/tornado ; \
	WIND_BASE=$$WIND_HOME ; \
	WIND_GNU_PATH=$$WIND_BASE/host ; \
	AR=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ar$${ARCH}.exe \
	CC=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/cc$${ARCH}.exe \
	LD=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ld$${ARCH}.exe \
	NM=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/nm$${ARCH}.exe \
	RANLIB=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ranlib$${ARCH}.exe \
	STRIP=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/strip$${ARCH}.exe \
	IFLAGS="-I$$WIND_BASE/target/h -I$$WIND_BASE/target/h/wrn/coreip" \
	BUILD_CC=/usr/bin/cc \
	BUILD_LD=/usr/bin/cc \
	SEARCH_PATH=/tornado ./configure --product=appweb --defaults=standard --type=DEBUG \
	--host=i386-wrs-vxworks \
	--enable-ranges --enable-multithread --enable-assert --enable-squeeze \
	--disable-floating-point --disable-legacy-api --disable-access-log \
	--disable-shared --disable-samples --disable-run-as-service \
	--without-ssl --without-matrixssl --without-php5 --without-openssl \
	--without-cgi --without-gacompat --disable-modules

vx6env:
	ARCH=pentium ; \
	WIND_HOME=c:/WindRiver ; \
	VXWORKS=vxworks-6.3 ; \
	WIND_BASE=$$WIND_HOME/$$VXWORKS ; \
	PLATFORM=i586-wrs-vxworks ; \
	WIND_GNU_PATH=$$WIND_HOME/gnu/3.4.4-vxworks-6.3 ; \
	AR=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/ar$${ARCH}.exe \
	CC=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/cc$${ARCH}.exe \
	LD=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/bin/cc$${ARCH}.exe \
	NM=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/$${PLATFORM}/bin/nm.exe \
	RANLIB=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/$${PLATFORM}/bin/ranlib.exe \
	STRIP=$$WIND_GNU_PATH/$$WIND_HOST_TYPE/$${PLATFORM}/bin/strip.exe \
	CFLAGS="-I$$WIND_BASE/target/h -I$$WIND_BASE/target/h/wrn/coreip" \
	BUILD_CC=/usr/bin/cc \
	BUILD_LD=/usr/bin/cc \
	./configure --product=appweb --defaults=standard --type=DEBUG --host=i386-wrs-vxworks \
	--enable-ranges --enable-multithread --enable-assert --enable-squeeze \
	--disable-floating-point --disable-legacy-api --disable-access-log \
	--disable-shared --disable-samples --disable-run-as-service \
	--without-ssl --without-matrixssl --without-php5 --without-openssl \
	--without-cgi --without-gacompat --disable-modules

#
#	Some useful configure targets for development
#
config:
	./configure --defaults=dev --enable-test --disable-samples # --enable-ipv6

cygwin:
	./configure --cygwin --defaults=dev --enable-test --disable-samples \
	--disable-modules --without-php5 --without-matrixssl --without-openssl --without-gacompat

config64:
	./configure --defaults=dev --enable-test --disable-samples --host x86_64-apple-darwin \
		--without-matrixssl --without-openssl --without-php5 # --enable-ipv6

release:
	./configure --defaults=release --enable-test

single:
	./configure --defaults=dev --enable-test --disable-samples --disable-modules --without-php5 --without-matrixssl --without-openssl --without-gacompat

static:
	./configure --defaults=dev --enable-test --disable-modules

freebsd:
	./configure --defaults=dev --enable-test --disable-samples --disable-modules --without-php5 --without-matrixssl --without-openssl --without-gacompat --disable-multithread

vxenv:
	wrenv -p vxworks-6.3 -f sh -o print_env

php:
	DIR=/Users/mob/svn/packages/php/php-5.2.0 ; \
	CC=arm-linux-gcc AR=arm-linux-ar LD=arm-linux-ld
	./configure -with-php5=builtin --with-php5-dir=$$DIR \
		--with-php5-iflags="-I$$DIR -I$$DIR/main -I$$DIR/Zend -I$$DIR/TSRM" \
		--with-php5-libpath="$$DIR/libs" --with-php5-libs="libphp5 crypt resolv db z"

#
#	Using ubuntu packages: uclibc-toolchain, libuclibc-dev
#	Use dpkg -L package to see installed files. Installed under /usr/i386-uclibc-linux
#
uclibc:
	PREFIX=i386-uclibc-linux; \
	DIR=/usr/i386-uclibc-linux/bin ; \
	AR=$${DIR}/$${PREFIX}-ar \
	CC=$${DIR}/$${PREFIX}-gcc \
	LD=$${DIR}/$${PREFIX}-gcc \
	NM=$${DIR}/$${PREFIX}-nm \
	RANLIB=$${DIR}/$${PREFIX}-ranlib \
	STRIP=$${DIR}/$${PREFIX}-strip \
	CFLAGS="-fno-stack-protector" \
	CXXFLAGS="-fno-rtti -fno-exceptions" \
	BUILD_CC=/usr/bin/cc \
	BUILD_LD=/usr/bin/cc \
	./configure --product=appweb --defaults=standard --host=i386-pc-linux \
	--type=RELEASE --enable-ranges --enable-multithread --enable-squeeze \
	--disable-assert --disable-floating-point --disable-legacy-api \
	--disable-access-log --disable-shared --disable-samples --disable-c-api-client \
	--disable-run-as-service --without-ssl --without-matrixssl --without-egi \
	--without-php5 --without-openssl --without-cgi --without-gacompat \
	--without-admin --without-c-api --without-put --without-upload \
	--disable-shared-libc --disable-modules

u:
	PREFIX=i386-uclibc-linux; \
	DIR=/usr/i386-uclibc-linux/bin ; \
	AR=$${DIR}/$${PREFIX}-ar \
	CC=$${DIR}/$${PREFIX}-gcc \
	LD=$${DIR}/$${PREFIX}-gcc \
	NM=$${DIR}/$${PREFIX}-nm \
	RANLIB=$${DIR}/$${PREFIX}-ranlib \
	STRIP=$${DIR}/$${PREFIX}-strip \
	CFLAGS="-fno-stack-protector" \
	CXXFLAGS="-fno-rtti -fno-exceptions" \
	BUILD_CC=/usr/bin/cc \
	BUILD_LD=/usr/bin/cc \
	./configure --product=appweb --defaults=dev --host=i386-pc-linux \
	--type=DEBUG --enable-squeeze

## Local variables:
## tab-width: 4
## End:
## vim: sw=4 ts=4
