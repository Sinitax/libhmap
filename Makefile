PREFIX ?= /usr/local
INCLDIR ?= /include
LIBDIR ?= /lib

CFLAGS = -Wno-prototype -Wunused-function -Wunused-variable -Wconversion
CFLAGS += -I include -I lib/liballoc/include

ifeq "$(DEBUG)" "1"
CFLAGS += -g
endif

ifeq "$(ASSERT_ARGS)" "1"
CFLAGS += -DLIBHMAP_ASSERT_ARGS=1
endif

ifeq "$(ASSERT_ALLOC)" "1"
CFLAGS += -DLIBHMAP_ASSERT_ALLLOC=1
endif

all: build/libhmap.so build/libhmap.a build/test

clean:
	rm -rf build

build:
	mkdir build

lib/liballoc/build/liballoc.a:
	make -C lib/liballoc build/liballoc.a

build/libhmap.a: src/hmap.c include/hmap.h libhmap.api | build
	$(CC) -o build/tmp.o src/hmap.c $(CFLAGS) -r
	objcopy --keep-global-symbols=libhmap.api build/tmp.o build/fixed.o
	$(AR) rcs $@ build/fixed.o

build/libhmap.so: src/hmap.c include/hmap.h libhmap.lds | build
	$(CC) -o $@ src/hmap.c -fPIC $(CFLAGS) -shared \
		-Wl,-version-script libhmap.lds

build/test: src/test.c build/libhmap.a lib/liballoc/build/liballoc.a  | build
	$(CC) -o $@ $^ -I include -I lib/liballoc/include -g

install:
	install -m 644 include/hmap.h -t "$(DESTDIR)$(PREFIX)$(INCLDIR)"
	install -m 755 build/libhmap.a -t "$(DESTDIR)$(PREFIX)$(LIBDIR)"
	install -m 755 build/libhmap.so -t "$(DESTDIR)$(PREFIX)$(LIBDIR)"

uninstall:
	rm -f "$(DESTDIR)$(PREFIX)$(INCLDIR)/hmap.h"
	rm -f "$(DESTDIR)$(PREFIX)$(LIBDIR)/libhmap.a"
	rm -f "$(DESTDIR)$(PREFIX)$(LIBDIR)/libhmap.so"

.PHONY: all clean install uninstall
