/*
 * Copyright (C) 2007 Free Software Foundation
 *
 * Author: Simon Josefsson
 *
 * This file is part of GNUTLS.
 *
 * The GNUTLS library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA
 *
 */

/* This file contains support functions for 'TLS Handshake Message for
 * Supplemental Data' (RFC 4680).
 *
 * The idea here is simple.  MHD__gnutls_handshake() in gnuts_handshake.c
 * will call MHD__gnutls_gen_supplemental and MHD__gnutls_parse_supplemental
 * when some extension requested that supplemental data be sent or
 * received.  Extension request this by setting the flags
 * do_recv_supplemental or do_send_supplemental in the session.
 *
 * The functions in this file iterate through the MHD__gnutls_supplemental
 * array, and calls the send/recv functions for each respective data
 * type.
 *
 * The receive function of each data type is responsible for decoding
 * its own data.  If the extension did not expect to receive
 * supplemental data, it should return GNUTLS_E_UNEXPECTED_PACKET.
 * Otherwise, it just parse the data as normal.
 *
 * The send function needs to append the 2-byte data format type, and
 * append the 2-byte length of its data, and the data.  If it doesn't
 * want to send any data, it is fine to return without doing anything.
 */

#include "gnutls_int.h"
#include "gnutls_supplemental.h"
#include "gnutls_errors.h"
#include "gnutls_num.h"

typedef int (*supp_recv_func) (MHD_gtls_session_t session,
                               const opaque * data, size_t data_size);
typedef int (*supp_send_func) (MHD_gtls_session_t session,
                               MHD_gtls_buffer * buf);

typedef struct
{
  const char *name;
  MHD_gnutls_supplemental_data_format_type_t type;
  supp_recv_func supp_recv_func;
  supp_send_func supp_send_func;
} MHD_gnutls_supplemental_entry;

MHD_gnutls_supplemental_entry MHD__gnutls_supplemental[] = {
  {0, 0, 0, 0}
};


static supp_recv_func
get_supp_func_recv (MHD_gnutls_supplemental_data_format_type_t type)
{
  MHD_gnutls_supplemental_entry *p;

  for (p = MHD__gnutls_supplemental; p->name != NULL; p++)
    if (p->type == type)
      return p->supp_recv_func;

  return NULL;
}

int
MHD__gnutls_gen_supplemental (MHD_gtls_session_t session,
                              MHD_gtls_buffer * buf)
{
  MHD_gnutls_supplemental_entry *p;
  int ret;

  /* Make room for 3 byte length field. */
  ret = MHD_gtls_buffer_append (buf, "\0\0\0", 3);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  for (p = MHD__gnutls_supplemental; p->name; p++)
    {
      supp_send_func supp_send = p->supp_send_func;
      size_t sizepos = buf->length;
      int ret;

      /* Make room for supplement type and length byte length field. */
      ret = MHD_gtls_buffer_append (buf, "\0\0\0\0", 4);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      ret = supp_send (session, buf);
      if (ret < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      /* If data were added, store type+length, otherwise reset. */
      if (buf->length > sizepos + 4)
        {
          buf->data[sizepos] = 0;
          buf->data[sizepos + 1] = p->type;
          buf->data[sizepos + 2] = ((buf->length - sizepos - 4) >> 8) & 0xFF;
          buf->data[sizepos + 3] = (buf->length - sizepos - 4) & 0xFF;
        }
      else
        buf->length -= 4;
    }

  buf->data[0] = ((buf->length - 3) >> 16) & 0xFF;
  buf->data[1] = ((buf->length - 3) >> 8) & 0xFF;
  buf->data[2] = (buf->length - 3) & 0xFF;

  MHD__gnutls_debug_log ("EXT[%x]: Sending %d bytes of supplemental data\n",
                         session, buf->length);

  return buf->length;
}

int
MHD__gnutls_parse_supplemental (MHD_gtls_session_t session,
                                const uint8_t * data, int datalen)
{
  const opaque *p = data;
  ssize_t dsize = datalen;
  size_t total_size;

  DECR_LEN (dsize, 3);
  total_size = MHD_gtls_read_uint24 (p);
  p += 3;

  if (dsize != total_size)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
    }

  do
    {
      uint16_t supp_data_type;
      uint16_t supp_data_length;
      supp_recv_func recv_func;

      DECR_LEN (dsize, 2);
      supp_data_type = MHD_gtls_read_uint16 (p);
      p += 2;

      DECR_LEN (dsize, 2);
      supp_data_length = MHD_gtls_read_uint16 (p);
      p += 2;

      MHD__gnutls_debug_log
        ("EXT[%x]: Got supplemental type=%02x length=%d\n", session,
         supp_data_type, supp_data_length);

      recv_func = get_supp_func_recv (supp_data_type);
      if (recv_func)
        {
          int ret = recv_func (session, p, supp_data_length);
          if (ret < 0)
            {
              MHD_gnutls_assert ();
              return ret;
            }
        }
      else
        {
          MHD_gnutls_assert ();
          return GNUTLS_E_RECEIVED_ILLEGAL_PARAMETER;
        }

      DECR_LEN (dsize, supp_data_length);
      p += supp_data_length;
    }
  while (dsize > 0);

  return 0;
}
