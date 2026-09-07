/*
 * Copyright (c) 2010  Aurelien Jacobs <aurel@gnuage.org>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef IPTV_AVCODEC_HTMLSUBTITLES_H
#define IPTV_AVCODEC_HTMLSUBTITLES_H

#include <stddef.h>

/* Converts SubRip / SAMI style markup to ASS. Writes at most dst_size-1
 * bytes into dst plus a terminating NUL; excess input is dropped. */
void ff_htmlmarkup_to_ass(void *log_ctx, char *dst, size_t dst_size, const char *in);

#endif /* AVCODEC_HTMLSUBTITLES_H */
