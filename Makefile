PREFIX ?= /usr/local
INCLDIR ?= /include
LIBDIR ?= /lib

CFLAGS = -I include -Wno-prototype -Wunused-function
CFLAGS += -Wunused-variable -Wconversion

ifeq "$(DEBUG)" "1"
CFLAGS += -g -DLIBMAP_CHECK_ENABLE=1
endif

all: build/libmap.so build/libmap.a build/test

clean:
	rm -rf build

build:
	mkdir build

build/libmap.a: src/hashmap.c include/hashmap.h | build
	$(CC) -o build/tmp.o src/hashmap.c $(CFLAGS) -r
	objcopy --keep-global-symbols=libhashmap.api build/tmp.o build/fixed.o
	$(AR) rcs $@ build/fixed.o

build/libmap.so: src/hashmap.c include/hashmap.h | build
	$(CC) -o $@ src/hashmap.c -fPIC $(CFLAGS) -shared \
		-Wl,-version-script libhashmap.lds

build/test: src/test.c build/libmap.a | build
	$(CC) -o $@ $^ -I include -g

install:
	install -m 644 include/hashmap.h -t "$(DESTDIR)$(PREFIX)$(INCLDIR)"
	install -m 755 build/libmap.a -t "$(DESTDIR)$(PREFIX)$(LIBDIR)"
	install -m 755 build/libmap.so -t "$(DESTDIR)$(PREFIX)$(LIBDIR)"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)$(INCLDIR)/hashmap.h"
	rm -f "$(DESTDIR)$(PREFIX)$(LIBDIR)/libmap.a"
	rm -f "$(DESTDIR)$(PREFIX)$(LIBDIR)/libmap.so"

.PHONY: all clean install uninstall
