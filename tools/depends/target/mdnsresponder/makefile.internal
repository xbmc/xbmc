DEFINES  ?= -DNOT_HAVE_SA_LEN -DUSES_NETLINK
INCLUDES ?= -I./mDNSShared -I./mDNSCore
PREFIX   ?= /usr/local
LIBDIR   ?= $(PREFIX)/lib
INCDIR   ?= $(PREFIX)/include
LIB =    libmDNSEmbedded.a

HEADERS = mDNSShared/dns_sd.h mDNSCore/mDnsEmbedded.h

OBJECTS =  mDNSShared/dnssd_clientshim.o mDNSPosix/mDNSPosix.o mDNSCore/mDNS.o
OBJECTS += mDNSCore/DNSCommon.o mDNSShared/mDNSDebug.o mDNSShared/GenLinkedList.o
OBJECTS += mDNSCore/uDNS.o mDNSShared/PlatformCommon.o mDNSPosix/mDNSUNP.o
OBJECTS += mDNSCore/DNSDigest.o mDNSCore/mDnsEmbedded.o mDNSShared/dnssd_clientlib.o
OBJECTS += mDNSCore/CryptoAlg.o

all: $(LIB)
install: $(LIBDIR)/$(LIB) $(addprefix $(INCDIR)/,$(HEADERS))

$(INCDIR)/%.h: %.h
	mkdir -p $(INCDIR)
	install -m 644 $< $(INCDIR)

$(LIBDIR)/$(LIB): $(LIB)
	mkdir -p $(LIBDIR)
	install -m 644 $< $@

$(LIB): $(OBJECTS)
	$(AR) rvs $@ $^

%.o: %.c
	$(CC) $(INCLUDES) $(DEFINES) $(CFLAGS) -c $< -o $@

clean:
	-rm $(OBJECTS) $(LIB)
