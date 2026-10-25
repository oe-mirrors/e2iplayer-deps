# Native build:  make            (needs the libpng headers)
# Cross build:   make CC="$(CROSS)gcc" CFLAGS="$TARGET_CFLAGS" LDFLAGS="$TARGET_LDFLAGS"
CC     ?= gcc
STRIP  ?= strip
LIB     = exteplayer3png.so
SRC     = $(wildcard src/*.c)

override CFLAGS  += -O2 -Wall -fPIC
override LDFLAGS += -shared

$(LIB): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC) $(LDFLAGS) -lpng -o $@
	-$(STRIP) --strip-unneeded $@

clean:
	rm -f $(LIB)

.PHONY: clean
