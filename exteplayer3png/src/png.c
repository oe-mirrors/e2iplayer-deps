/*
 * exteplayer3png - PNG writer plugin of exteplayer3
 *
 * exteplayer3 dlopen()s "exteplayer3png.so" to save graphic subtitles
 * (PGS / DVB / XSUB) as PNG files when it runs with -G <directory>
 * (external/plugins/src/png.c there). Without the plugin those subtitle
 * tracks cannot be shown. Based on github.com/e2iplayer/exteplayer3png
 * (GPLv2 or later, version 1).
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <png.h>

#define EXTEPLAYER3PNG_VERSION "1.1"
/* GetVersion() of the original plugin returns the major version */
#define EXTEPLAYER3PNG_VERSION_MAJOR 1

/* libpng's own limit for width and height */
#define MAX_SIZE 1000000

int GetVersion(void);
int SaveRGBAImage(const char *filename, const unsigned char *data, int width, int height);

int GetVersion(void)
{
    return EXTEPLAYER3PNG_VERSION_MAJOR;
}

/* stderr of exteplayer3 is E2iPlayer's message channel: one short line per
 * error instead of libpng's default output, warnings are dropped */
static void on_error(png_structp png, png_const_charp msg)
{
    fprintf(stderr, "exteplayer3png: %s\n", msg);
    png_longjmp(png, 1);
}

static void on_warning(png_structp png, png_const_charp msg)
{
    (void)png;
    (void)msg;
}

static int write_png(FILE *fp, const unsigned char *data, int width, int height)
{
    png_structp png;
    png_infop info;
    int y;

    png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, on_error, on_warning);
    if (!png)
        return -1;
    info = png_create_info_struct(png);
    if (!info) {
        png_destroy_write_struct(&png, NULL);
        return -1;
    }
    /* libpng errors jump back here; png and info are not changed after this point */
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        return -1;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGBA,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    /* subtitle pictures are flat colours on transparency: without a row filter
     * and with the fastest zlib level they get smallest per CPU time (the
     * default adaptive filtering tries all five filters on every row) */
    png_set_filter(png, PNG_FILTER_TYPE_BASE, PNG_FILTER_NONE);
    png_set_compression_level(png, 1);
    png_write_info(png, info);
    /* row by row straight from the caller's buffer: no row pointer table */
    for (y = 0; y < height; y++)
        png_write_row(png, (png_const_bytep)(data + (size_t)y * width * 4));
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    return 0;
}

/* writes width x height RGBA pixels (8 bit per channel, rows without padding)
 * as a PNG file; 0 on success, -1 on error (no file is left behind then) */
int SaveRGBAImage(const char *filename, const unsigned char *data, int width, int height)
{
    FILE *fp;
    int ret;

    if (!filename || !data || width <= 0 || height <= 0 || width > MAX_SIZE || height > MAX_SIZE)
        return -1;

    fp = fopen(filename, "wb");
    if (!fp)
        return -1;
    ret = write_png(fp, data, width, height);
    /* a full disk may only show up when the buffered data is written */
    if (fclose(fp) != 0)
        ret = -1;
    if (ret)
        remove(filename);
    return ret;
}
