SRCS=AddonsOperations.cpp \
     ApplicationOperations.cpp \
     AudioLibrary.cpp \
     FavouritesOperations.cpp \
     FileItemHandler.cpp \
     FileOperations.cpp \
     GUIOperations.cpp \
     InputOperations.cpp \
     JSONRPC.cpp \
     JSONServiceDescription.cpp \
     PlayerOperations.cpp \
     PlaylistOperations.cpp \
     ProfilesOperations.cpp \
     PVROperations.cpp \
     SettingsOperations.cpp \
     SystemOperations.cpp \
     TextureOperations.cpp \
     VideoLibrary.cpp \
     XBMCOperations.cpp \

LIB=json-rpc.a

GENERATED_JSON = ServiceDescription.h
GENERATED_ADDON_JSON = ../../../addons/xbmc.json/addon.xml
JSON_VERSION_SRC = schema/version.txt
JSON_SRC =  $(JSON_VERSION_SRC) schema/license.txt schema/methods.json schema/types.json schema/notifications.json

all: $(GENERATED_JSON) $(GENERATED_ADDON_JSON) $(LIB)

-include ../../../Makefile.include
ifeq ($(wildcard $(JSON_BUILDER)),)
  JSON_BUILDER = $(shell which JsonSchemaBuilder)
ifeq ($(JSON_BUILDER),)
  JSON_BUILDER = ../../../tools/depends/native/JsonSchemaBuilder/bin/JsonSchemaBuilder
endif
endif

$(GENERATED_JSON): $(JSON_SRC)
	$(JSON_BUILDER) $(JSON_SRC)

.PHONY: $(notdir $(GENERATED_ADDON_JSON))
$(notdir $(GENERATED_ADDON_JSON)): $(GENERATED_ADDON_JSON)

$(GENERATED_ADDON_JSON): $(JSON_VERSION_SRC)
	sed s/@jsonrpc_version@/`cat $(JSON_VERSION_SRC)`/g $@.in > $@

-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
