PREFIX ?= /usr/local
INCLDIR ?= /include
LIBDIR ?= /lib

CFLAGS = -I include -Wno-prototype -Wunused-function
CFLAGS += -Wunused-variable -Wconversion

ifeq "$(DEBUG)" "1"
CFLAGS += -g
endif

all: build/libhashmap.so build/libhashmap.a build/test

clean:
	rm -rf build

build:
	mkdir build

build/libhashmap.a: src/hashmap.c include/hashmap.h | build
	$(CC) -o build/tmp.o src/hashmap.c $(CFLAGS) -r
	objcopy --keep-global-symbols=libhashmap.api build/tmp.o build/fixed.o
	$(AR) rcs $@ build/fixed.o

build/libhashmap.so: src/hashmap.c include/hashmap.h | build
	$(CC) -o $@ src/hashmap.c -fPIC $(CFLAGS) -shared \
		-Wl,-version-script libhashmap.lds

build/test: src/test.c build/libhashmap.a | build
	$(CC) -o $@ $^ -I include -g

install:
	install -m 644 include/hashmap.h -t "$(DESTDIR)$(PREFIX)$(INCLDIR)"
	install -m 755 build/libhashmap.a -t "$(DESTDIR)$(PREFIX)$(LIBDIR)"
	install -m 755 build/libhashmap.so -t "$(DESTDIR)$(PREFIX)$(LIBDIR)"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)$(INCLDIR)/hashmap.h"
	rm -f "$(DESTDIR)$(PREFIX)$(LIBDIR)/libhashmap.a"
	rm -f "$(DESTDIR)$(PREFIX)$(LIBDIR)/libhashmap.so"

.PHONY: all clean install uninstall
