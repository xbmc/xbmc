SUBDIRS = taocrypt src examples testsuite
EXTRA_DIST = yassl.dsp yassl.dsw certs/*.pem certs/*.d* certs/*.txt doc/*.html \
	doc/*.png doc/*.gif doc/*.css stunnel/stunnel.* stunnel/*.txt \
    stunnel/src/*.c lib/dummy FLOSS-EXCEPTIONS yassl.vcproj yassl.sln *.m4


lib-only:
	cd taocrypt/src; make; cd ../../src; make; cd ../

openssl-links:
	cd lib; ln -s ../taocrypt/src/libtaocrypt.a libcrypto.a; \
    ln -s ../src/libyassl.a libssl.a; cd ../
    
