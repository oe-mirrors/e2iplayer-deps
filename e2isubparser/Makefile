# Python 3 C extension.
#
# Native build:  make
# Cross build:   make CC="$(CROSS)gcc" \
#                     PYTHON_INCLUDES="-I<sysroot>/usr/include/python3.x" \
#                     CFLAGS="$TARGET_CFLAGS" LDFLAGS="$TARGET_LDFLAGS"
#
# PYTHON_INCLUDES defaults to `python3-config --includes`; override it for
# cross builds (or pass PYTHON_CONFIG=<target>-python3-config).

CC             ?= gcc
STRIP          ?= strip
PYTHON_CONFIG  ?= python3-config
PYTHON_INCLUDES ?= $(shell $(PYTHON_CONFIG) --includes)

MODULE   = _subparser.so

# expat/ (2.6.4+) or the legacy expat-2.2.0/ - whichever is present
EXPATDIR = $(firstword $(wildcard src/expat src/expat-2.2.0))

SRC = src/subparsermodule.c \
      src/vlc/src/subtitle.c \
      src/ffmpeg/src/htmlsubtitles.c \
      $(EXPATDIR)/xmlparse.c $(EXPATDIR)/xmlrole.c $(EXPATDIR)/xmltok.c \
      $(wildcard src/ttml/src/*.c) \
      $(wildcard src/html/src/*.c)

INCLUDES = -Isrc/vlc/include -Isrc/ffmpeg/include -I$(EXPATDIR) \
           -Isrc/ttml/include -Isrc/html/include $(PYTHON_INCLUDES)

override CFLAGS += -Os -fPIC -fno-strict-aliasing \
                   -ffunction-sections -fdata-sections \
                   -DNDEBUG -DHAVE_EXPAT_CONFIG_H \
                   -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE
override LDFLAGS += -Wl,--gc-sections

# llroundf() from the TTML/HTML code - the OE recipe links it explicitly too
LDLIBS ?= -lm

$(MODULE): $(SRC)
	$(CC) -shared $(CFLAGS) $(CPPFLAGS) $(INCLUDES) $(SRC) $(LDFLAGS) -o $@ $(LDLIBS)
	-$(STRIP) -s $@

clean:
	rm -f $(MODULE)

.PHONY: clean
