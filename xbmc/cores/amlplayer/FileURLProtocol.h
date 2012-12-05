#pragma once
/*
 *      Copyright (C) 2011-2012 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <stdint.h>
#include <stdio.h>

// this is an icky hack, we need to use amffmpeg structures but we cannot
// include them or risk colliding with our internal ffmpeg defs. The only
// this we use is URLProtocol to setup callbacks into uur vfs, so we fake
// a renamed URLContext/URLProtocol. The only thing we need to worry about
// is keeping the alignment to what is in amffmpeg, so we can forward
// declare all pointers to stucts as we never ref them and the compiler is happy.

struct AML_AVClass;
struct url_lpbuf;

typedef struct AML_URLContext {
  const  AML_AVClass *av_class;
  struct AML_URLProtocol *prot;
  struct url_lpbuf *lpbuf;
  int flags;
  int is_streamed;
  int max_packet_size;
  void *priv_data;
  char *filename;
  char *headers;
  int is_connected;
  int is_slowmedia;
  int fastdetectedinfo;
  int support_time_seek;
  char *location;
} AML_URLContext;

typedef struct AML_URLProtocol {
  const char *name;
  int (*url_open)(AML_URLContext *h, const char *url, int flags);
  int (*url_read)(AML_URLContext *h, unsigned char *buf, int size);
  int (*url_write)(AML_URLContext *h,  unsigned char *buf, int size);
  int64_t (*url_seek)(AML_URLContext *h, int64_t pos, int whence);
  int64_t (*url_exseek)(AML_URLContext *h, int64_t pos, int whence);
  int (*url_close)(AML_URLContext *h);
  struct AML_URLContext *next;
  int (*url_read_pause)(AML_URLContext *h, int pause);
  int64_t (*url_read_seek)(AML_URLContext *h, int stream_index, int64_t timestamp, int flags);
  int (*url_get_file_handle)(AML_URLContext *h);
  int priv_data_size;
  const AML_AVClass *priv_data_class;
  int flags;
  int (*url_check)(AML_URLContext *h, int mask);
} AML_URLProtocol;

class CFileURLProtocol
{

public:
  static int Open (AML_URLContext *h, const char *filename, int flags);
  static int Read (AML_URLContext *h, unsigned char *buf, int size);
  static int Write(AML_URLContext *h, unsigned char *buf, int size);
  static int64_t Seek(AML_URLContext *h, int64_t pos, int whence);
  static int64_t SeekEx(AML_URLContext *h, int64_t pos, int whence);
  static int Close(AML_URLContext *h);
};
