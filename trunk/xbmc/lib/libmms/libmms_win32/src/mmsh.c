/*
 * Copyright (C) 2002-2003 the xine project
 *
 * This file is part of xine, a free video player.
 *
 * xine is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * xine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id: mmsh.c,v 1.16 2007/12/11 20:50:43 jwrdegoede Exp $
 *
 * MMS over HTTP protocol
 *   written by Thibaut Mattern
 *   based on mms.c and specs from avifile
 *   (http://avifile.sourceforge.net/asf-1.0.htm)
 *
 * TODO:
 *   error messages
 *   http support cleanup, find a way to share code with input_http.c (http.h|c)
 *   http proxy support
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>

#ifdef G_OS_WIN32
#ifdef _MSC_VER
#include <Winsock2.h>
#endif
#include <ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#define LOG_MODULE "mmsh"
#define LOG_VERBOSE
#ifdef DEBUG
# define lprintf g_print
#else
# define lprintf(x)
#endif

/* cheat a bit and call ourselves mms.c to keep the code in mmsio.h clean */
#define __MMS_C__

#include "bswap.h"
#include "mmsh.h"
#include "asfheader.h"
#include "uri.h"
#include "utils.h"

/* #define USERAGENT "User-Agent: NSPlayer/7.1.0.3055\r\n" */
#define USERAGENT "User-Agent: NSPlayer/4.1.0.3856\r\n"
#define CLIENTGUID "Pragma: xClientGUID={c77e7400-738a-11d2-9add-0020af0a3278}\r\n"


#define MMSH_PORT                  80
#define MMSH_UNKNOWN                0
#define MMSH_SEEKABLE               1
#define MMSH_LIVE                   2

#define CHUNK_HEADER_LENGTH         4
#define EXT_HEADER_LENGTH           8
#define CHUNK_TYPE_RESET       0x4324
#define CHUNK_TYPE_DATA        0x4424
#define CHUNK_TYPE_END         0x4524
#define CHUNK_TYPE_ASF_HEADER  0x4824
#define CHUNK_SIZE              65536  /* max chunk size */
#define ASF_HEADER_SIZE     (8192 * 2)  /* max header size */

#define SCRATCH_SIZE             1024

static const char* mmsh_FirstRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s:%d\r\n"
    "Pragma: no-cache,rate=1.000000,stream-time=0,stream-offset=0:0,request-context=%u,max-duration=0\r\n"
    CLIENTGUID
    "Connection: Close\r\n\r\n";

static const char* mmsh_SeekableRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s:%d\r\n"
    "Pragma: no-cache,rate=1.000000,stream-time=%u,stream-offset=%u:%u,request-context=%u,max-duration=%u\r\n"
    CLIENTGUID
    "Pragma: xPlayStrm=1\r\n"
    "Pragma: stream-switch-count=%d\r\n"
    "Pragma: stream-switch-entry=%s\r\n" /*  ffff:1:0 ffff:2:0 */
    "Connection: Close\r\n\r\n";

static const char* mmsh_LiveRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s:%d\r\n"
    "Pragma: no-cache,rate=1.000000,request-context=%u\r\n"
    "Pragma: xPlayStrm=1\r\n"
    CLIENTGUID
    "Pragma: stream-switch-count=%d\r\n"
    "Pragma: stream-switch-entry=%s\r\n"
    "Connection: Close\r\n\r\n";


#if 0
/* Unused requests */
static const char* mmsh_PostRequest =
    "POST %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s\r\n"
    "Pragma: client-id=%u\r\n"
/*    "Pragma: log-line=no-cache,rate=1.000000,stream-time=%u,stream-offset=%u:%u,request-context=2,max-duration=%u\r\n" */
    "Pragma: Content-Length: 0\r\n"
    CLIENTGUID
    "\r\n";

static const char* mmsh_RangeRequest =
    "GET %s HTTP/1.0\r\n"
    "Accept: */*\r\n"
    USERAGENT
    "Host: %s:%d\r\n"
    "Range: bytes=%Lu-\r\n"
    CLIENTGUID
    "Connection: Close\r\n\r\n";
#endif



/* 
 * mmsh specific types 
 */


struct mmsh_s {

  /* FIXME: de-xine-ification */
  void *custom_data;

  int           s;

  /* url parsing */
  char         *url;
  char         *proxy_url;
  char         *proto;
  char         *connect_host;
  int           connect_port;
  char         *http_host;
  int           http_port;
  int           http_request_number;
  char         *proxy_user;
  char         *proxy_password;
  char         *host_user;
  char         *host_password;
  char         *uri;

  char          str[SCRATCH_SIZE]; /* scratch buffer to built strings */

  int           stream_type;  /* seekable or broadcast */
  
  /* receive buffer */
  
  /* chunk */
  uint16_t      chunk_type;
  uint16_t      chunk_length;
  uint32_t      chunk_seq_number;
  uint8_t       buf[CHUNK_SIZE];

  int           buf_size;
  int           buf_read;

  uint8_t       asf_header[ASF_HEADER_SIZE];
  uint32_t      asf_header_len;
  uint32_t      asf_header_read;
  int           num_stream_ids;
  int           stream_ids[ASF_MAX_NUM_STREAMS];
  int           stream_types[ASF_MAX_NUM_STREAMS];
  uint32_t      packet_length;
  int64_t       file_length;
  uint64_t      time_len; /* playback time in 100 nanosecs (10^-7) */
  uint64_t      preroll;
  uint64_t      asf_num_packets;
  char          guid[37];
  uint32_t      bitrates[ASF_MAX_NUM_STREAMS];
  uint32_t      bitrates_pos[ASF_MAX_NUM_STREAMS];

  int           has_audio;
  int           has_video;
  int           seekable;

  off_t         current_pos;
  int           user_bandwidth;
};

static int fallback_io_select(void *data, int socket, int state, int timeout_msec)
{
  int ret;
  fd_set set;
  struct timeval tv = { timeout_msec / 1000, (timeout_msec % 1000) * 1000};
  FD_ZERO(&set);
  FD_SET(socket, &set);
  ret = select(1, (state == MMS_IO_READ_READY) ? &set : NULL,
      (state == MMS_IO_WRITE_READY) ? &set : NULL, NULL, &tv);

#ifdef G_OS_WIN32
  if (ret > 0)
    return MMS_IO_STATUS_READY;
  else if (ret == 0)
    return MMS_IO_STATUS_TIMEOUT;
  else
#endif
  return ret;
}

static off_t fallback_io_read(void *data, int socket, char *buf, off_t num)
{
  off_t len = 0, ret;
/*   lprintf("%d\n", fallback_io_select(data, socket, MMS_IO_READ_READY, 1000)); */
  errno = 0;
  while (len < num)
  {
    ret = (off_t)recv(socket, buf + len, num - len, 0);
    if(ret == 0)
      break; /* EOF */
    if(ret < 0)
      switch(errno)
      {
	  case EAGAIN:
	    lprintf("len == %lld\n", (gint64) len);
	    break;
	  default:
	    lprintf("len == %lld\n", (gint64) len);
	    perror(NULL);
	    /* if already read something, return it, we will fail next time */
	    return len ? len : ret; 
      }
    len += ret;
  }
  lprintf("ret len == %lld\nnum == %lld\n", (gint64) len, (gint64) num);
  lprintf("read\n");
  return len;
}

static off_t fallback_io_write(void *data, int socket, char *buf, off_t num)
{
  return (off_t)send(socket, buf, num, 0);
}

static int fallback_io_tcp_connect(void *data, const char *host, int port)
{
  
  struct hostent *h;
  int i, s;
  
  h = gethostbyname(host);
  if (h == NULL) {
/*     fprintf(stderr, "unable to resolve host: %s\n", host); */
    return -1;
  }

  s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);  
  if (s == -1) {
/*     fprintf(stderr, "failed to create socket: %s", strerror(errno)); */
    return -1;
  }

#ifndef G_OS_WIN32
  if (fcntl (s, F_SETFL, fcntl (s, F_GETFL) & ~O_NONBLOCK) == -1) {
/*     _x_message(stream, XINE_MSG_CONNECTION_REFUSED, "can't put socket in non-blocking mode", strerror(errno), NULL); */
    return -1;
  }
#endif

  for (i = 0; h->h_addr_list[i]; i++) {
    struct in_addr ia;
    struct sockaddr_in sin;
 
    memcpy (&ia, h->h_addr_list[i], 4);
    sin.sin_family = AF_INET;
    sin.sin_addr   = ia;
    sin.sin_port   = htons(port);
    
#ifdef G_OS_WIN32
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) ==-1) {
#else
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) ==-1 && errno != EINPROGRESS) {
#endif

      /* FIXME: de-xine-ification */
/*       _x_message(stream, XINE_MSG_CONNECTION_REFUSED, strerror(errno), NULL); */
      close(s);
      continue;
    }
    
    return s;
  }
  return -1;
}


static mms_io_t fallback_io =
  {
    &fallback_io_select,
    NULL,
    &fallback_io_read,
    NULL,
    &fallback_io_write,
    NULL,
    &fallback_io_tcp_connect,
    NULL,
  };

static mms_io_t default_io =   {
    &fallback_io_select,
    NULL,
    &fallback_io_read,
    NULL,
    &fallback_io_write,
    NULL,
    &fallback_io_tcp_connect,
    NULL,
  };


#define io_read(io, socket, buf, num) \
  ((io) ? (io)->read(io->read_data, socket, buf, num) \
   : default_io.read(NULL , socket, buf, num))
#define io_write(io, socket, buf, num) \
  ((io) ? (io)->write(io->write_data, socket, buf, num) \
   : default_io.write(NULL , socket, buf, num))
#define io_select(io, fd, state, timeout_msec) \
  ((io) ? (io)->select(io->select_data, fd, state, timeout_msec) \
   : default_io.select(NULL , fd, state, timeout_msec))
#define io_connect(io, host, port) \
  ((io) ? (io)->connect(io->connect_data, host, port) \
   : default_io.connect(NULL , host, port))


static int get_guid (unsigned char *buffer, int offset) {
  int i;
  GUID g;
  
  g.Data1 = LE_32(buffer + offset);
  g.Data2 = LE_16(buffer + offset + 4);
  g.Data3 = LE_16(buffer + offset + 6);
  for(i = 0; i < 8; i++) {
    g.Data4[i] = buffer[offset + 8 + i];
  }
  
  for (i = 1; i < GUID_END; i++) {
    if (!memcmp(&g, &guids[i].guid, sizeof(GUID))) {
      lprintf ("GUID: %s\n", guids[i].name);

      return i;
    }
  }

  lprintf ("libmmsh: unknown GUID: 0x%x, 0x%x, 0x%x, "
           "{ 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx, 0x%hx }\n",
          g.Data1, g.Data2, g.Data3,
          g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3], 
          g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
  return GUID_ERROR;
}

static int send_command (mms_io_t *io, mmsh_t *this, char *cmd)  {
  int length;

  lprintf ("send_command:\n%s\n", cmd);

  length = strlen(cmd);
  if (io_write(io, this->s, cmd, length) != length) {
    lprintf ("mmsh: send error.\n");
    return 0;
  }
  return 1;
}

static int get_answer (mms_io_t *io, mmsh_t *this) {
 
  int done, len, linenum;
  char *features;

  lprintf ("get_answer\n");

  done = 0; len = 0; linenum = 0;
  this->stream_type = MMSH_UNKNOWN;

  while (!done) {

    if (io_read(io, this->s, &(this->buf[len]), 1) != 1) {
      lprintf ("mmsh: alart: end of stream\n");
      return 0;
    }

    if (this->buf[len] == '\012') {

      this->buf[len] = '\0';
      len--;
      
      if ((len >= 0) && (this->buf[len] == '\015')) {
        this->buf[len] = '\0';
        len--;
      }

      linenum++;
      
      lprintf ("answer: >%s<\n", this->buf);

      if (linenum == 1) {
        int httpver, httpsub, httpcode;
        char httpstatus[51];

        if (sscanf(this->buf, "HTTP/%d.%d %d %50[^\015\012]", &httpver, &httpsub,
            &httpcode, httpstatus) != 4) {
	  lprintf ("mmsh: bad response format\n");
          return 0;
        }

        if (httpcode >= 300 && httpcode < 400) {
	  lprintf ("mmsh: 3xx redirection not implemented: >%d %s<\n", httpcode, httpstatus);
          return 0;
        }

        if (httpcode < 200 || httpcode >= 300) {
	  lprintf ("mmsh: http status not 2xx: >%d %s<\n", httpcode, httpstatus);
          return 0;
        }
      } else {

        if (!g_strncasecmp(this->buf, "Location: ", 10)) {
	  lprintf ("mmsh: Location redirection not implemented.\n");
          return 0;
        }
        
        if (!g_strncasecmp(this->buf, "Pragma:", 7)) {
          features = strstr(this->buf + 7, "features=");
          if (features) {
            if (strstr(features, "seekable")) {
              lprintf("seekable stream\n");
              this->stream_type = MMSH_SEEKABLE;
              this->seekable = 1;
            } else {
              if (strstr(features, "broadcast")) {
                lprintf("live stream\n");
                this->stream_type = MMSH_LIVE;
                this->seekable = 0;
              }
            }
          }
        }
      }
      
      if (len == -1) {
        done = 1;
      } else {
        len = 0;
      }
    } else {
      len ++;
    }
  }
  if (this->stream_type == MMSH_UNKNOWN) {
    lprintf ("mmsh: unknown stream type\n");
    this->stream_type = MMSH_SEEKABLE; /* FIXME ? */
    this->seekable = 1;
  }
  return 1;
}

static int get_chunk_header (mms_io_t *io, mmsh_t *this) {
  uint8_t chunk_header[CHUNK_HEADER_LENGTH];
  uint8_t ext_header[EXT_HEADER_LENGTH];
  int read_len;
  int ext_header_len;

  lprintf ("get_chunk_header\n");

  /* read chunk header */
  read_len = io_read(io, this->s, chunk_header, CHUNK_HEADER_LENGTH);
  if (read_len != CHUNK_HEADER_LENGTH) {
    lprintf ("chunk header read failed, %d != %d\n", read_len, CHUNK_HEADER_LENGTH);
    return 0;
  }
  this->chunk_type       = LE_16 (&chunk_header[0]);
  this->chunk_length     = LE_16 (&chunk_header[2]);

  switch (this->chunk_type) {
    case CHUNK_TYPE_DATA:
      ext_header_len = 8;
      break;
    case CHUNK_TYPE_END:
      ext_header_len = 4;
      break;
    case CHUNK_TYPE_ASF_HEADER:
      ext_header_len = 8;
      break;
    case CHUNK_TYPE_RESET:
      ext_header_len = 4;
      break;
    default:
      ext_header_len = 0;
  }
  /* read extended header */
  if (ext_header_len > 0) {
    read_len = io_read (io, this->s, ext_header, ext_header_len);
    if (read_len != ext_header_len) {
      lprintf ("extended header read failed. %d != %d\n", read_len, ext_header_len);
      return 0;
    }
  }
  
  if (this->chunk_type == CHUNK_TYPE_DATA || this->chunk_type == CHUNK_TYPE_END)
    this->chunk_seq_number = LE_32 (&ext_header[0]);

  /* display debug infos */
#ifdef DEBUG
  switch (this->chunk_type) {
    case CHUNK_TYPE_DATA:
      lprintf ("chunk type:       CHUNK_TYPE_DATA\n");
      lprintf ("chunk length:     %d\n", this->chunk_length);
      lprintf ("chunk seq:        %d\n", this->chunk_seq_number);
      lprintf ("unknown:          %d\n", ext_header[4]);
      lprintf ("mmsh seq:         %d\n", ext_header[5]);
      lprintf ("len2:             %d\n", LE_16(&ext_header[6]));
      break;
    case CHUNK_TYPE_END:
      lprintf ("chunk type:       CHUNK_TYPE_END\n");
      lprintf ("continue: %d\n", this->chunk_seq_number);
      break;
    case CHUNK_TYPE_ASF_HEADER:
      lprintf ("chunk type:       CHUNK_TYPE_ASF_HEADER\n");
      lprintf ("chunk length:     %d\n", this->chunk_length);
      lprintf ("unknown:          %2X %2X %2X %2X %2X %2X\n",
	       ext_header[0], ext_header[1], ext_header[2], ext_header[3],
	       ext_header[4], ext_header[5]);
      lprintf ("len2:             %d\n", LE_16(&ext_header[6]));
      break;
    case CHUNK_TYPE_RESET:
      lprintf ("chunk type:       CHUNK_TYPE_RESET\n");
      lprintf ("chunk seq:        %d\n", this->chunk_seq_number);
      lprintf ("unknown:          %2X %2X %2X %2X\n",
	       ext_header[0], ext_header[1], ext_header[2], ext_header[3]);
      break;
    default:
      lprintf ("unknown chunk:    %4X\n", this->chunk_type);
  }
#endif

  this->chunk_length -= ext_header_len;
  return 1;
}

static int get_header (mms_io_t *io, mmsh_t *this) {
  int len = 0;

  lprintf("get_header\n");

  this->asf_header_len = 0;
  this->asf_header_read = 0;

  /* read chunk */
  while (1) {
    if (get_chunk_header(io, this)) {
      if (this->chunk_type == CHUNK_TYPE_ASF_HEADER) {
        if ((this->asf_header_len + this->chunk_length) > ASF_HEADER_SIZE) {
	  lprintf ("mmsh: the asf header exceed %d bytes\n", ASF_HEADER_SIZE);
          return 0;
        } else {
	  len = io_read(io, this->s, this->asf_header + this->asf_header_len,
			this->chunk_length);
          this->asf_header_len += len;
          if (len != this->chunk_length) {
            return 0;
          }
        }
      } else {
        break;
      }
    } else {
      lprintf("get_chunk_header failed\n");
      return 0;
    }
  }

  if (this->chunk_type == CHUNK_TYPE_DATA) {
    /* read the first data chunk */
    len = io_read (io, this->s, this->buf, this->chunk_length);

    if (len != this->chunk_length) {
      return 0;
    } else {
      /* check and 0 pad the first data chunk */
      if (this->chunk_length > this->packet_length) {
        lprintf ("mmsh: chunk_length(%d) > packet_length(%d)\n",
                 this->chunk_length, this->packet_length);
        return 0;
      }

      /* explicit padding with 0 */
      if (this->chunk_length < this->packet_length)
        memset(this->buf + this->chunk_length, 0,
               this->packet_length - this->chunk_length);

      this->buf_size = this->packet_length;

      return 1;
    }
  } else {
    /* unexpected packet type */
    return 0;
  }
}

static void interp_header (mms_io_t *io, mmsh_t *this) {

  int i;

  lprintf ("interp_header, header_len=%d\n", this->asf_header_len);

  this->packet_length = 0;
  this->num_stream_ids = 0;
  this->asf_num_packets = 0;

  /*
   * parse asf header
   */

  i = 30;
  while ((i + 24) < this->asf_header_len) {

    int guid;
    uint64_t length;

    guid = get_guid(this->asf_header, i);
    i += 16;

    length = LE_64(this->asf_header + i);
    i += 8;

    if ((i + length) >= this->asf_header_len) return;

    switch (guid) {

      case GUID_ASF_FILE_PROPERTIES:

        this->packet_length = LE_32(this->asf_header + i + 92 - 24);
        if (this->packet_length > CHUNK_SIZE) {
          this->packet_length = 0;
          break;
        }
        this->file_length   = LE_64(this->asf_header + i + 40 - 24);
        this->time_len       = LE_64(this->asf_header + i + 64 - 24);
        //this->time_len       = LE_64(this->asf_header + i + 72 - 24);
        this->preroll        = LE_64(this->asf_header + i + 80 - 24);
        lprintf ("file object, packet length = %d (%d)\n",
		 this->packet_length, LE_32(this->asf_header + i + 96 - 24));
        break;

      case GUID_ASF_STREAM_PROPERTIES:
        {
	  uint16_t flags;
          uint16_t stream_id;
          int      type;
	  int      encrypted;

          guid = get_guid(this->asf_header, i);
          switch (guid) {
            case GUID_ASF_AUDIO_MEDIA:
              type = ASF_STREAM_TYPE_AUDIO;
              this->has_audio = 1;
              break;

            case GUID_ASF_VIDEO_MEDIA:
	    case GUID_ASF_JFIF_MEDIA:
	    case GUID_ASF_DEGRADABLE_JPEG_MEDIA:
              type = ASF_STREAM_TYPE_VIDEO;
              this->has_video = 1;
              break;

            case GUID_ASF_COMMAND_MEDIA:
              type = ASF_STREAM_TYPE_CONTROL;
              break;

            default:
              type = ASF_STREAM_TYPE_UNKNOWN;
          }

	  flags = LE_16(this->asf_header + i + 48);
	  stream_id = flags & 0x7F;
	  encrypted = flags >> 15;

          lprintf ("stream object, stream id: %d, type: %d, encrypted: %d\n",
		   stream_id, type, encrypted);

          this->stream_types[stream_id] = type;
          this->stream_ids[this->num_stream_ids] = stream_id;
          this->num_stream_ids++;

        }
        break;

      case GUID_ASF_STREAM_BITRATE_PROPERTIES:
        {
          uint16_t streams = LE_16(this->asf_header + i);
          uint16_t stream_id;
          int j;

	  lprintf ("stream bitrate properties\n");
          lprintf ("streams %d\n", streams);

          for(j = 0; j < streams; j++) {
            stream_id = LE_16(this->asf_header + i + 2 + j * 6);

            lprintf ("stream id %d\n", stream_id);

            this->bitrates[stream_id] = LE_32(this->asf_header + i + 4 + j * 6);
            this->bitrates_pos[stream_id] = i + 4 + j * 6;
	    lprintf ("mmsh: stream id %d, bitrate %d\n", stream_id, this->bitrates[stream_id]);
          }
        }
        break;

      case GUID_ASF_DATA:
        this->asf_num_packets = LE_64(this->asf_header + i + 40 - 24);
        lprintf("mmsh: num_packets: %d\n", (int)this->asf_num_packets);
        break;

      default:
        lprintf ("unknown object\n");
        break;
    }

    lprintf ("length    : %lld\n", length);

    if (length > 24) {
      i += length - 24;
    }
  }
}

const static char *const mmsh_proto_s[] = { "mms", "mmsh", NULL };

static int mmsh_valid_proto (char *proto) {
  int i = 0;

  lprintf("mmsh_valid_proto\n");

  if (!proto)
    return 0;

  while(mmsh_proto_s[i]) {
    if (!g_strcasecmp(proto, mmsh_proto_s[i])) {
      return 1;
    }
    i++;
  }
  return 0;
}

/*
 * returns 1 on error
 */
static int mmsh_tcp_connect(mms_io_t *io, mmsh_t *this) {
  int progress, res;
  
  if (!this->connect_port) this->connect_port = MMSH_PORT;
  
  /* 
   * try to connect 
   */
  lprintf("try to connect to %s on port %d \n", this->connect_host, this->connect_port);

  this->s = io_connect (io, this->connect_host, this->connect_port);

  if (this->s == -1) {
    lprintf ("mmsh: failed to connect '%s'\n", this->connect_host);
    return 1;
  }

  /* connection timeout 15s */
  progress = 0;
  do {
//    report_progress(this->stream, progress);
    res = io_select (io, this->s, MMS_IO_WRITE_READY, 500);
    progress += 1;
  } while ((res == MMS_IO_STATUS_TIMEOUT) && (progress < 30));
  if (res != MMS_IO_STATUS_READY) {
    close (this->s);
    this->s = -1;
    return 1;
  }
  lprintf ("connected\n");

  return 0;
}

static int mmsh_connect_int (mms_io_t *io, mmsh_t *this, off_t seek, uint32_t time_seek) {
  int    i;
  int    video_stream = -1;
  int    audio_stream = -1;
  int    max_arate    = -1;
  int    min_vrate    = -1;
  int    min_bw_left  = 0;
  int    stream_id;
  int    bandwitdh_left;
  char   stream_selection[10 * ASF_MAX_NUM_STREAMS]; /* 10 chars per stream */
  int    offset;
  
  /* Close exisiting connection (if any) and connect */
  if (this->s != -1)
    close(this->s);

  if (mmsh_tcp_connect(io, this)) {
    return 0;
  }
//  report_progress (stream, 30);

  /*
   * let the negotiations begin...
   */
  this->num_stream_ids = 0;

  /* first request */
  lprintf("first http request\n");
  
  g_snprintf (this->str, SCRATCH_SIZE, mmsh_FirstRequest, this->uri,
            this->http_host, this->http_port, this->http_request_number++);

  if (!send_command (io, this, this->str))
    goto fail;

  if (!get_answer (io, this))
    goto fail;

    
  get_header(io, this);
  interp_header(io, this);
  if (!this->packet_length || !this->num_stream_ids)
    goto fail;
  
  close(this->s);
//  report_progress (stream, 20);

  
  /* choose the best quality for the audio stream */
  /* i've never seen more than one audio stream */
  for (i = 0; i < this->num_stream_ids; i++) {
    stream_id = this->stream_ids[i];
    switch (this->stream_types[stream_id]) {
      case ASF_STREAM_TYPE_AUDIO:
        if ((audio_stream == -1) || (this->bitrates[stream_id] > max_arate)) {
          audio_stream = stream_id;
          max_arate = this->bitrates[stream_id];
        }
        break;
      default:
        break;
    }
  }

  /* choose a video stream adapted to the user bandwidth */
  bandwitdh_left = this->user_bandwidth - max_arate;
  if (bandwitdh_left < 0) {
    bandwitdh_left = 0;
  }
  lprintf("bandwitdh %d, left %d\n", this->user_bandwidth, bandwitdh_left);

  min_bw_left = bandwitdh_left;
  for (i = 0; i < this->num_stream_ids; i++) {
    stream_id = this->stream_ids[i];
    switch (this->stream_types[stream_id]) {
      case ASF_STREAM_TYPE_VIDEO:
        if (((bandwitdh_left - this->bitrates[stream_id]) < min_bw_left) &&
            (bandwitdh_left >= this->bitrates[stream_id])) {
          video_stream = stream_id;
          min_bw_left = bandwitdh_left - this->bitrates[stream_id];
        }
        break;
      default:
        break;
    }
  }  

  /* choose the stream with the lower bitrate */
  if ((video_stream == -1) && this->has_video) {
    for (i = 0; i < this->num_stream_ids; i++) {
      stream_id = this->stream_ids[i];
      switch (this->stream_types[stream_id]) {
        case ASF_STREAM_TYPE_VIDEO:
          if ((video_stream == -1) || 
              (this->bitrates[stream_id] < min_vrate) ||
              (!min_vrate)) {
            video_stream = stream_id;
            min_vrate = this->bitrates[stream_id];
          }
          break;
        default:
          break;
      }
    }
  }

  lprintf("audio stream %d, video stream %d\n", audio_stream, video_stream);
  
  /* second request */
  lprintf("second http request\n");

  if (mmsh_tcp_connect(io, this)) {
    return 0;
  }

  /* stream selection string */
  /* The same selection is done with mmst */
  /* 0 means selected */
  /* 2 means disabled */
  offset = 0;
  for (i = 0; i < this->num_stream_ids; i++) {
    int size;
    if ((this->stream_ids[i] == audio_stream) ||
        (this->stream_ids[i] == video_stream)) {
      size = g_snprintf(stream_selection + offset, sizeof(stream_selection) - offset,
                      "ffff:%d:0 ", this->stream_ids[i]);
    } else {
      lprintf ("disabling stream %d\n", this->stream_ids[i]);
      size = g_snprintf(stream_selection + offset, sizeof(stream_selection) - offset,
                      "ffff:%d:2 ", this->stream_ids[i]);
    }
    if (size < 0) goto fail;
    offset += size;
  }

  switch (this->stream_type) {
    case MMSH_SEEKABLE:
      g_snprintf (this->str, SCRATCH_SIZE, mmsh_SeekableRequest, this->uri,
                this->http_host, this->http_port, time_seek,
                (unsigned int)(seek >> 32),
                (unsigned int)seek, this->http_request_number++, 0,
                this->num_stream_ids, stream_selection);
      break;
    case MMSH_LIVE:
      g_snprintf (this->str, SCRATCH_SIZE, mmsh_LiveRequest, this->uri,
                this->http_host, this->http_port, this->http_request_number++,
                this->num_stream_ids, stream_selection);
      break;
  }
  
  if (!send_command (io, this, this->str))
    goto fail;
  
  lprintf("before read \n");

  if (!get_answer (io, this))
    goto fail;

  if (!get_header(io, this))
    goto fail;

  interp_header(io, this);
  if (!this->packet_length || !this->num_stream_ids)
    goto fail;
  
  for (i = 0; i < this->num_stream_ids; i++) {
    if ((this->stream_ids[i] != audio_stream) &&
        (this->stream_ids[i] != video_stream)) {
      lprintf("disabling stream %d\n", this->stream_ids[i]);

      /* forces the asf demuxer to not choose this stream */
      if (this->bitrates_pos[this->stream_ids[i]]) {
	this->asf_header[this->bitrates_pos[this->stream_ids[i]]]     = 0;
	this->asf_header[this->bitrates_pos[this->stream_ids[i]] + 1] = 0;
	this->asf_header[this->bitrates_pos[this->stream_ids[i]] + 2] = 0;
	this->asf_header[this->bitrates_pos[this->stream_ids[i]] + 3] = 0;
      }
    }
  }
  return 1;
fail:
  close(this->s);
  this->s = -1;
  return 0;
}

mmsh_t *mmsh_connect (mms_io_t *io, void *data, const char *url, int bandwidth) {
  mmsh_t *this;
  GURI  *uri = NULL;
  GURI  *proxy_uri = NULL;
  char  *proxy_env;
  if (!url)
    return NULL;

  mms_sock_init();

//  report_progress (stream, 0);
  /*
   * initializatoin is essential here.  the fail: label depends
   * on the various char * in our this structure to be
   * NULL if they haven't been assigned yet.
   */
  this = (mmsh_t*) malloc (sizeof (mmsh_t));
  this->url=NULL;
  this->proxy_url = NULL;
  this->proto = NULL;
  this->connect_host = NULL;
  this->http_host = NULL;
  this->proxy_user = NULL;
  this->proxy_password = NULL;
  this->host_user = NULL;
  this->host_password = NULL;
  this->uri = NULL;

  this->custom_data     = data;
  this->url             = strdup(url);
  if ((proxy_env = getenv("http_proxy")) != NULL)
    this->proxy_url = strdup(proxy_env);
  else
    this->proxy_url = NULL;
  this->s               = -1;
  this->asf_header_len  = 0;
  this->asf_header_read = 0;
  this->num_stream_ids  = 0;
  this->packet_length   = 0;
  this->buf_size        = 0;
  this->buf_read        = 0;
  this->has_audio       = 0;
  this->has_video       = 0;
  this->current_pos     = 0;
  this->user_bandwidth  = bandwidth;
  this->http_request_number = 1;

//  report_progress (stream, 0);

  if (this->proxy_url) {
    proxy_uri = gnet_uri_new(this->proxy_url);
    if (!proxy_uri) {
      lprintf("invalid proxy url\n");
      goto fail;
    }
    if (! proxy_uri->port ) {
      proxy_uri->port = 3128; //default squid port
    }
  }
  uri = gnet_uri_new(this->url);
  if (!uri) {
    lprintf ("invalid url\n");
    goto fail;
  }
  if (! uri->port ) {
    //checked in tcp_connect, but it's better to initialize it here
    uri->port = MMSH_PORT;
  }
  if (this->proxy_url) {
    char * uri_string;
    this->proto = (uri->scheme) ? strdup(uri->scheme) : NULL;
    this->connect_host = (proxy_uri->hostname) ? strdup(proxy_uri->hostname) : NULL;
    this->connect_port = proxy_uri->port;
    this->http_host = (uri->scheme) ? strdup(uri->hostname) : NULL;
    this->http_port = uri->port;
    this->proxy_user = (proxy_uri->user) ? strdup(proxy_uri->user) : NULL;
    this->proxy_password = (proxy_uri->passwd) ? strdup(proxy_uri->passwd) : NULL;
    this->host_user = (uri->user) ? strdup(uri->user) : NULL;
    this->host_password = (uri->passwd) ? strdup(uri->passwd) : NULL;
    gnet_uri_set_scheme(uri,"http");
    this->uri = gnet_mms_helper(uri);
    g_free(uri_string);
  } else {
    this->proto = (uri->scheme) ? strdup(uri->scheme) : NULL;
    this->connect_host = (uri->hostname) ? strdup(uri->hostname) : NULL;
    this->connect_port = uri->port;
    this->http_host = (uri->hostname) ? strdup(uri->hostname) : NULL;
    this->http_port = uri->port;
    this->proxy_user = NULL;
    this->proxy_password = NULL;
    this->host_user =(uri->user) ?  strdup(uri->user) : NULL;
    this->host_password = (uri->passwd) ? strdup(uri->passwd) : NULL;
    this->uri = gnet_mms_helper(uri);
  }

  if(!this->uri)
	goto fail;

  if (proxy_uri) {
    gnet_uri_delete(proxy_uri);
    proxy_uri = NULL;
  }
  if (uri) {
    gnet_uri_delete(uri);
    uri = NULL;
  }
  if (!mmsh_valid_proto(this->proto)) {
    lprintf ("unsupported protocol\n");
    goto fail;
  }

  if (!mmsh_connect_int(io, this, 0, 0))
    goto fail;

//  report_progress (stream, 100);

  lprintf("mmsh_connect: passed\n" );

  return this;

fail:
  lprintf("mmsh_connect: failed\n" );
  if (proxy_uri)
    gnet_uri_delete(proxy_uri);
  if (uri)
    gnet_uri_delete(uri);
  if (this->s != -1)
    close(this->s);
  if (this->url)
    free(this->url);
  if (this->proxy_url)
    free(this->proxy_url);
  if (this->proto)
    free(this->proto);
  if (this->connect_host)
    free(this->connect_host);
  if (this->http_host)
    free(this->http_host);
  if (this->proxy_user)
    free(this->proxy_user);
  if (this->proxy_password)
    free(this->proxy_password);
  if (this->host_user)
    free(this->host_user);
  if (this->host_password)
    free(this->host_password);
  if (this->uri)
    free(this->uri);

  free(this);

  lprintf("mmsh_connect: failed return\n" );
  return NULL;
}


/*
 * returned value:
 *  0: error
 *  1: data packet read
 *  2: new header and data packet read
 */
static int get_media_packet (mms_io_t *io, mmsh_t *this) {
  int len = 0;

  lprintf("get_media_packet: this->packet_length: %d\n", this->packet_length);

  if (get_chunk_header(io, this)) {
    switch (this->chunk_type) {
      case CHUNK_TYPE_END:
	/* this->chunk_seq_number:
	 *     0: stop
	 *     1: a new stream follows
	 */
	if (this->chunk_seq_number == 0)
	  return 0;

        this->http_request_number = 1;
	if (!mmsh_connect_int (io, this, 0, 0))
	  return 0;

	/* What todo with: current_pos ??
	   Also our chunk_seq_numbers will probably restart from 0!
	   If this happens with a seekable stream (does it ever?)
	   and we get a seek request after this were fscked! */
        this->seekable = 0;

	/* mmsh_connect_int reads the first data packet */
	/* this->buf_size is set by mmsh_connect_int */
	return 2;

      case CHUNK_TYPE_DATA:
	/* nothing to do */
        break;

      case CHUNK_TYPE_RESET:
        /* next chunk is an ASF header */

	if (this->chunk_length != 0) {
	  /* that's strange, don't know what to do */
	  return 0;
	}
	if (!get_header (io, this))
	  return 0;
	interp_header(io, this);

	/* What todo with: current_pos ??
	   Also our chunk_seq_numbers might restart from 0!
	   If this happens with a seekable stream (does it ever?) 
	   and we get a seek request after this were fscked! */
        this->seekable = 0;

	/* get_header reads the first data packet */
	/* this->buf_size is set by get_header */
	return 2;

      default:
	lprintf ("mmsh: unexpected chunk type\n");
        return 0;
    }

    len = io_read (io, this->s, this->buf, this->chunk_length);
      
    if (len == this->chunk_length) {
      /* explicit padding with 0 */
      if (this->chunk_length > this->packet_length) {
	lprintf ("mmsh: chunk_length(%d) > packet_length(%d)\n",
		 this->chunk_length, this->packet_length);
	return 0;
      }

      {
	char *base  = (char *)(this->buf);
	char *start = base + this->chunk_length;
	char *end   = start + this->packet_length - this->chunk_length;
	if ((start > base) && (start < (base+CHUNK_SIZE-1)) &&
	    (start < end)  && (end < (base+CHUNK_SIZE-1))) {
	  memset(start, 0,
		 this->packet_length - this->chunk_length);
	}
	if (this->packet_length > CHUNK_SIZE) {
	  this->buf_size = CHUNK_SIZE;
	} else {
	  this->buf_size = this->packet_length;
	}
      }
      return 1;
    } else {
      lprintf ("mmsh: read error, %d != %d\n", len, this->chunk_length);
      return 0;
    }
  } else {
    return 0;
  }
}

int mmsh_peek_header (mmsh_t *this, char *data, int maxsize) {
  int len;

  lprintf("mmsh_peek_header\n");

  len = (this->asf_header_len < maxsize) ? this->asf_header_len : maxsize;

  memcpy(data, this->asf_header, len);
  return len;
}

int mmsh_read (mms_io_t *io, mmsh_t *this, char *data, int len) {
  int total;

  total = 0;

  lprintf ("mmsh_read: len: %d\n", len);

  /* Check if the stream didn't get closed because of previous errors */
  if (this->s == -1)
    return total;

  while (total < len) {

    if (this->asf_header_read < this->asf_header_len) {
      int n, bytes_left ;

      bytes_left = this->asf_header_len - this->asf_header_read ;

      if ((len-total) < bytes_left)
	n = len-total;
      else
	n = bytes_left;

      memcpy (&data[total], &this->asf_header[this->asf_header_read], n);

      this->asf_header_read += n;
      total += n;
      this->current_pos += n;
    } else {

      int n, bytes_left ;

      bytes_left = this->buf_size - this->buf_read;

      if (bytes_left == 0) {
	int packet_type;

	this->buf_size=this ->buf_read = 0;
	packet_type = get_media_packet (io, this);

	if (packet_type == 0) {
	  lprintf ("mmsh: get_media_packet failed\n");
	  return total;
	} else if (packet_type == 2) {
	  continue;
	}
	bytes_left = this->buf_size;
      }

      if ((len-total) < bytes_left)
	n = len-total;
      else
	n = bytes_left;

      memcpy (&data[total], &this->buf[this->buf_read], n);

      this->buf_read += n;
      total += n;
      this->current_pos += n;
    }
  }
  return total;
}

off_t mmsh_seek (mms_io_t *io, mmsh_t *this, off_t offset, int origin) {
  off_t dest;
  off_t dest_packet_seq;
  uint32_t orig_asf_header_len = this->asf_header_len;
  uint32_t orig_asf_packet_len = this->packet_length;
  
  if (!this->seekable)
    return this->current_pos;
  
  switch (origin) {
    case SEEK_SET:
      dest = offset;
      break;
    case SEEK_CUR:
      dest = this->current_pos + offset;
      break;
    case SEEK_END:
      dest = mmsh_get_length (this) + offset;
    default:
      return this->current_pos;
  }

  dest_packet_seq = dest - this->asf_header_len;
  dest_packet_seq = dest_packet_seq >= 0 ?
    dest_packet_seq / this->packet_length : -1;

  if (dest_packet_seq < 0) {
    if (this->chunk_seq_number > 0) {
      lprintf("mmsh: seek within header, already read beyond first packet, resetting connection\n");
      if (!mmsh_connect_int(io, this, 0, 0)) {
        /* Oops no more connection let our caller know things are fscked up */
        return this->current_pos = -1;
      }
      /* Some what simple / naive check to check for changed headers
         if the header was changed we are once more fscked up */
      if (this->asf_header_len != orig_asf_header_len ||
          this->packet_length  != orig_asf_packet_len) {
        lprintf("mmsh: AIIEEE asf header or packet length changed on re-open for seek\n");
        /* Its a different stream, so its useless! */
        close (this->s);
        this->s = -1;
        return this->current_pos = -1;
      }
    } else
      lprintf("mmsh: seek within header, resetting buf_read\n");

    // reset buf_read
    this->buf_read = 0;
    this->asf_header_read = dest;
    return this->current_pos = dest;
  }

  // dest_packet_seq >= 0
  if (this->asf_num_packets && dest == this->asf_header_len +
      this->asf_num_packets*this->packet_length) {
    // Requesting the packet beyond the last packet, can cause the server to
    // not return any packet or any eos command.  This can cause
    // mms_packet_seek() to hang.
    // This is to allow seeking at end of stream, and avoid hanging.
    --dest_packet_seq;
    lprintf("mmsh: seek to eos!\n");
  }

  if (dest_packet_seq != this->chunk_seq_number) {

    if (this->asf_num_packets && dest_packet_seq >= this->asf_num_packets) {
      // Do not seek beyond the last packet.
      return this->current_pos;
    }
    
    lprintf("mmsh: seek to %d, packet: %d\n", (int)dest, (int)dest_packet_seq);
    if (!mmsh_connect_int(io, this, (dest_packet_seq+1) * this->packet_length, 0)) {
      /* Oops no more connection let our caller know things are fscked up */
      return this->current_pos = -1;
    }
    /* Some what simple / naive check to check for changed headers
       if the header was changed we are once more fscked up */
    if (this->asf_header_len != orig_asf_header_len ||
        this->packet_length  != orig_asf_packet_len) {
      lprintf("mmsh: AIIEEE asf header or packet length changed on re-open for seek\n");
      /* Its a different stream, so its useless! */
      close (this->s);
      this->s = -1;
      return this->current_pos = -1;
    }
  }
  else
    lprintf("mmsh: seek within current packet, dest: %d, current pos: %d\n",
       (int)dest, (int)this->current_pos);

  /* make sure asf_header is seen as fully read by mmsh_read() this is needed
     in case our caller tries to seek over part of the header, or when we've
     done an actual packet seek as get_header() resets asf_header_read then. */
  this->asf_header_read = this->asf_header_len;

  /* check we got what we want */
  if (dest_packet_seq == this->chunk_seq_number) {
    this->buf_read = dest -
      (this->asf_header_len + dest_packet_seq*this->packet_length);
    this->current_pos = dest;
  } else {
    lprintf("Seek failed, wanted packet: %d, got packet: %d\n",
      (int)dest_packet_seq, (int)this->chunk_seq_number);
    this->buf_read = 0;
    this->current_pos = this->asf_header_len + this->chunk_seq_number *
      this->packet_length;
  }

  lprintf("current_pos after seek to %d: %d (buf_read %d)\n",
    (int)dest, (int)this->current_pos, (int)this->buf_read);

  return this->current_pos;
}

int mmsh_time_seek (mms_io_t *io, mmsh_t *this, double time_sec) {
  uint32_t orig_asf_header_len = this->asf_header_len;
  uint32_t orig_asf_packet_len = this->packet_length;

  if (!this->seekable)
    return 0;

  lprintf("mmsh: time seek to %f secs\n", time_sec);
  if (!mmsh_connect_int(io, this, 0, time_sec * 1000 + guint64_to_gdouble(this->preroll))) {
    /* Oops no more connection let our caller know things are fscked up */
    this->current_pos = -1;
    return 0;
  }
  /* Some what simple / naive check to check for changed headers
     if the header was changed we are once more fscked up */
  if (this->asf_header_len != orig_asf_header_len ||
      this->packet_length  != orig_asf_packet_len) {
    lprintf("mmsh: AIIEEE asf header or packet length changed on re-open for seek\n");
    /* Its a different stream, so its useless! */
    close (this->s);
    this->s = -1;
    this->current_pos = -1;
    return 0;
  }

  this->asf_header_read = this->asf_header_len;
  this->buf_read = 0;
  this->current_pos = this->asf_header_len + this->chunk_seq_number *
    this->packet_length;
  
  lprintf("mmsh, current_pos after time_seek:%d\n", (int)this->current_pos);

  return 1;
}

void mmsh_close (mmsh_t *this) {

  lprintf("mmsh_close\n");

  if (this->s != -1)
    close(this->s);
  if (this->url)
    free(this->url);
  if (this->proxy_url)
    free(this->proxy_url);
  if (this->proto)
    free(this->proto);
  if (this->connect_host)
    free(this->connect_host);
  if (this->http_host)
    free(this->http_host);
  if (this->proxy_user)
    free(this->proxy_user);
  if (this->proxy_password)
    free(this->proxy_password);
  if (this->host_user)
    free(this->host_user);
  if (this->host_password)
    free(this->host_password);
  if (this->uri)
    free(this->uri);
  if (this)
    free (this);
}


uint32_t mmsh_get_length (mmsh_t *this) {
  /* we could / should return this->file_len here, but usually this->file_len
     is longer then the calculation below, as usually an asf file contains an
     asf index object after the data stream. However since we do not have a
     (known) way to get to this index object through mms, we return a
     calculated size of what we can get to when we know. */
  if (this->asf_num_packets)
    return this->asf_header_len + this->asf_num_packets*this->packet_length;
  else
    return this->file_length;
}

double mmsh_get_time_length (mmsh_t *this) {
  return guint64_to_gdouble(this->time_len) / 1e7;
}

uint64_t mmsh_get_raw_time_length (mmsh_t *this) {
  return this->time_len;
}

off_t mmsh_get_current_pos (mmsh_t *this) {
  return this->current_pos;
}

uint32_t mmsh_get_asf_header_len (mmsh_t *this) {
  return this->asf_header_len;
}

uint32_t mmsh_get_asf_packet_len (mmsh_t *this) {
  return this->packet_length;
}

int mmsh_get_seekable (mmsh_t *this) {
  return this->seekable;
}
