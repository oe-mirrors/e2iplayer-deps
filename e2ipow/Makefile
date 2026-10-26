# Native build:  make
# Cross build:   make CC="$(CROSS)gcc" CFLAGS="$TARGET_CFLAGS" LDFLAGS="$TARGET_LDFLAGS"
CC     ?= gcc
STRIP  ?= strip
BIN     = e2ipow
SRC     = $(wildcard src/*.c)

override CFLAGS  += -O2 -Wall
override CFLAGS  += -ffunction-sections -fdata-sections
override LDFLAGS += -Wl,--gc-sections

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(SRC) $(LDFLAGS) -o $@
	-$(STRIP) -s $@

clean:
	rm -f $(BIN)

.PHONY: clean
