# Native build:  make
# Cross build:   make CC="$(CROSS)gcc" CFLAGS="$TARGET_CFLAGS" LDFLAGS="$TARGET_LDFLAGS"
CC     ?= gcc
STRIP  ?= strip
BIN     = lsdir
SRC     = $(wildcard src/*.c)

override CFLAGS  += -O2 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_LARGEFILE64_SOURCE=1
override CFLAGS  += -ffunction-sections -fdata-sections
override LDFLAGS += -Wl,--gc-sections

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC) $(LDFLAGS) -o $@
	-$(STRIP) -s $@

clean:
	rm -f $(BIN)

.PHONY: clean
