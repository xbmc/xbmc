# plugindir is set in configure

ROOTDIR    = ../../../..
CC = g++
CPP = g++
INCLUDES = -I./ -I/usr/include
INCLUDES += -I$(ROOTDIR)/include
INCLUDES += -I$(ROOTDIR)/include/link
INCLUDES += -I$(ROOTDIR)/linux_lib/ldil

BCMDEC_CFLAGS = $(INCLUDES) -D__LINUX_USER__ -DWMV_FILE_HANDLING
BCMDEC_CFLAGS += -O2  -g -Wall

BCMDEC_LDFLAGS = -L$(ROOTDIR)/linux_lib/ldil -lcrystalhd

##############################################################################
# change libgstplugin.la to something more suitable, e.g. libmysomething.la  #
##############################################################################
plugin_LTLIBRARIES = libgstbcmdec.la

##############################################################################
# for the next set of variables, rename the prefix if you renamed the .la,   #
#  e.g. libgstplugin_la_SOURCES => libgstbcmdec_la_SOURCES                 #
#       libgstplugin_la_CFLAGS  => libgstbcmdec_la_CFLAGS                  #
#       libgstplugin_la_LIBADD  => libgstbcmdec_la_LIBADD                  #
#       libgstplugin_la_LDFLAGS => libgstbcmdec_la_LDFLAGS                 #
##############################################################################

# sources used to compile this plug-in
libgstbcmdec_la_SOURCES = gstbcmdec.c decif.c parse.c

# flags used to compile this plugin
# add other _CFLAGS and _LIBS as needed
libgstbcmdec_la_CFLAGS = $(GST_CFLAGS) $(BCMDEC_CFLAGS)
libgstbcmdec_la_LIBADD = $(GST_LIBS) 
libgstbcmdec_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS) $(BCMDEC_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstbcmdec.h decif.h parse.h
