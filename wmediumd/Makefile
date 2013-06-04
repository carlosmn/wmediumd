VERSION_STR="\"0.1\""

# Look for libnl libraries
PKG_CONFIG ?= pkg-config
NL2FOUND := $(shell $(PKG_CONFIG) --atleast-version=2 libnl-2.0 && echo Y)
NL3FOUND := $(shell $(PKG_CONFIG) --atleast-version=3 libnl-3.0 && echo Y)
NL31FOUND := $(shell $(PKG_CONFIG) --exact-version=3.1 libnl-3.1 && echo Y)
NL3xFOUND := $(shell $(PKG_CONFIG) --atleast-version=3.2 libnl-3.0 && echo Y)

ifeq ($(NL2FOUND),Y)
CFLAGS += -DCONFIG_LIBNL20
LDFLAGS += -lnl-genl
NLLIBNAME = libnl-2.0
endif

ifeq ($(NL3xFOUND),Y)
# libnl 3.2 might be found as 3.2 and 3.0
NL3FOUND = N
CFLAGS += -DCONFIG_LIBNL30
LDFLAGS += -lnl-genl-3
NLLIBNAME = libnl-3.0
endif

ifeq ($(NL3FOUND),Y)
CFLAGS += -DCONFIG_LIBNL30
LDFLAGS += -lnl-genl
NLLIBNAME = libnl-3.0
endif

# nl-3.1 has a broken libnl-gnl-3.1.pc file
# as show by pkg-config --debug --libs --cflags --exact-version=3.1 libnl-genl-3.1;echo $?
ifeq ($(NL31FOUND),Y)
CFLAGS += -DCONFIG_LIBNL30
LDFLAGS += -lnl-genl
NLLIBNAME = libnl-3.1
endif

ifeq ($(NLLIBNAME),)
$(error Cannot find development files for any supported version of libnl)
endif

LDFLAGS += $(shell $(PKG_CONFIG) --libs $(NLLIBNAME))
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(NLLIBNAME))
NLVERSION :=$(shell $(PKG_CONFIG) --print-provides $(NLLIBNAME))

GLIBNAME = "glib-2.0"
LDFLAGS += $(shell $(PKG_CONFIG) --libs $(GLIBNAME))
CFLAGS += $(shell $(PKG_CONFIG) --cflags $(GLIBNAME))

CFLAGS+=-DVERSION_STR=$(VERSION_STR) -g
LDFLAGS+=-lconfig
OBJECTS=probability.o mac_address.o config.o proto.o
HEADERS=proto.h wmediumd.h

all: wmediumd dispatcher test write-config proto-test

wmediumd: wmediumd.o $(OBJECTS) $(HEADERS)
	$(CC) -o $@ wmediumd.o $(OBJECTS) $(LDFLAGS)

#relayer: relayer.o
#	$(CC) -o $@ relayer.o -lzmq

test: test.o
	$(CC) -o $@ test.o mac_address.o

proto-test: proto-test.o proto.o $(HEADERS)
	$(CC) -o $@ proto-test.o proto.o

dispatcher: dispatcher.o $(OBJECTS) $(HEADERS)
	$(CC) -o $@ dispatcher.o $(OBJECTS) $(LDFLAGS)

write-config: write-config.o config.o $(HEADERS)
	$(CC) -o $@ write-config.o $(OBJECTS) $(LDFLAGS)

clean: 
	rm -f $(OBJECTS) wmediumd.o wmediumd relayer
