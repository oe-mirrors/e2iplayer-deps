/* expat_config.h for the e2iplayer vendored copy of Expat.

   Hand-written (Expat is normally configured by autotools/CMake, which we
   do not run here). Kept deliberately small: only what lib/xmlparse.c,
   lib/xmltok.c and lib/xmlrole.c actually look at.

   Vendored Expat version: 2.6.4.  The directory is still called
   "expat-2.2.0" so that the out-of-tree OE build recipe keeps working
   without a path change.                                              */

#ifndef EXPAT_CONFIG_H
#define EXPAT_CONFIG_H 1

/* Byte order, derived from the compiler instead of hard-coding
   little-endian as the old 2.2.0 config did (big-endian enigma2 boxes
   still exist). GCC and Clang always define __BYTE_ORDER__.           */
#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__)              \
    && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#  define BYTEORDER 4321
#else
#  define BYTEORDER 1234
#endif

/* Standard headers / libc features present on every target we build for
   (Linux/glibc, Linux/musl, macOS). */
#define HAVE_DLFCN_H 1
#define HAVE_FCNTL_H 1
#define HAVE_INTTYPES_H 1
#define HAVE_STDINT_H 1
#define HAVE_STDLIB_H 1
#define HAVE_STRING_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_STAT_H 1
#define HAVE_SYS_TYPES_H 1
#define HAVE_UNISTD_H 1
#define HAVE_MEMMOVE 1
#define HAVE_BCOPY 1
#define HAVE_GETPAGESIZE 1
#define HAVE_MMAP 1

/* Entropy source for the hash-collision-attack salt. /dev/urandom is
   present on every enigma2 box (and on the CI runners); Expat falls back
   to it after trying getrandom(2). On Windows Expat uses rand_s()
   automatically. */
#define XML_DEV_URANDOM 1

/* Parser feature switches - same choices as the previous 2.2.0 config. */
#define XML_CONTEXT_BYTES 1024
#define XML_DTD 1
#define XML_GE 1
#define XML_NS 0

/* Package identification. */
#define PACKAGE_BUGREPORT "expat-bugs@libexpat.org"
#define PACKAGE_NAME "expat"
#define PACKAGE_STRING "expat 2.6.4"
#define PACKAGE_TARNAME "expat"
#define PACKAGE_VERSION "2.6.4"
#define PACKAGE_URL ""
#define VERSION PACKAGE_VERSION

#endif /* EXPAT_CONFIG_H */
