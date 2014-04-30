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
JSON_SRC =  schema/version.txt schema/license.txt schema/methods.json schema/types.json schema/notifications.json

all: $(GENERATED_JSON) $(LIB)

-include ../../../Makefile.include
ifeq ($(wildcard $(JSON_BUILDER)),)
  JSON_BUILDER = ../../../tools/depends/native/JsonSchemaBuilder/JsonSchemaBuilder
endif

$(GENERATED_JSON): $(JSON_SRC)
	$(JSON_BUILDER) $(JSON_SRC)

-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
