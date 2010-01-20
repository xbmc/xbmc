PP = cpp -P -I..

COMMON_C = ../asap.c ../acpu.c ../apokeysnd.c
COMMON_H = ../asap.h ../asap_internal.h ../anylang.h ../players.h

all: asap2wav.js asapweb.js

include ../players/Makefile

asap2wav.js: asap2wav.ppjs asap.ppjs $(COMMON_C) $(COMMON_H)
	$(PP) $< > $@

asapweb.js: asapweb.ppjs asap.ppjs $(COMMON_C) $(COMMON_H)
	$(PP) $< > $@

clean:
	rm -f asap2wav.js asapweb.js

.DELETE_ON_ERROR:
