/*
 * Copyright (C) 2002, 2003, 2004, 2005 Free Software Foundation
 *
 * Author: Nikos Mavrogiannopoulos
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

#include "gnutls_int.h"
#include "gnutls_auth_int.h"
#include "gnutls_errors.h"
#include "gnutls_num.h"
#include <ext_server_name.h>

/*
 * In case of a server: if a NAME_DNS extension type is received then it stores
 * into the session the value of NAME_DNS. The server may use MHD_gnutls_ext_get_server_name(),
 * in order to access it.
 *
 * In case of a client: If a proper NAME_DNS extension type is found in the session then
 * it sends the extension to the peer.
 *
 */

int
MHD_gtls_server_name_recv_params (MHD_gtls_session_t session,
                                  const opaque * data, size_t _data_size)
{
  int i;
  const unsigned char *p;
  uint16_t len, type;
  ssize_t data_size = _data_size;
  int server_names = 0;

  DECR_LENGTH_RET (data_size, 2, 0);
  len = MHD_gtls_read_uint16 (data);

  if (len != data_size)
    {
      /* This is unexpected packet length, but
       * just ignore it, for now.
       */
      MHD_gnutls_assert ();
      return 0;
    }

  p = data + 2;

  /* Count all server_names in the packet. */
  while (data_size > 0)
    {
      DECR_LENGTH_RET (data_size, 1, 0);
      p++;

      DECR_LEN (data_size, 2);
      len = MHD_gtls_read_uint16 (p);
      p += 2;

      /* make sure supplied server name is not empty */
      if (len > 0)
        {
          DECR_LENGTH_RET (data_size, len, 0);
          server_names++;
          p += len;
        }
      else
        {
#if HAVE_MESSAGES
          MHD__gnutls_handshake_log
            ("HSK[%x]: Received zero size server name (under attack?)\n",
             session);
#endif
        }
    }

  /* we cannot accept more server names. */
  if (server_names > MAX_SERVER_NAME_EXTENSIONS)
    {
#if HAVE_MESSAGES
      MHD__gnutls_handshake_log
        ("HSK[%x]: Too many server names received (under attack?)\n",
         session);
#endif
      server_names = MAX_SERVER_NAME_EXTENSIONS;
    }

  session->security_parameters.extensions.server_names_size = server_names;
  if (server_names == 0)
    return 0;                   /* no names found */

  p = data + 2;
  for (i = 0; i < server_names; i++)
    {
      type = *p;
      p++;

      len = MHD_gtls_read_uint16 (p);
      p += 2;

      switch (type)
        {
        case 0:                /* NAME_DNS */
          if (len <= MAX_SERVER_NAME_SIZE)
            {
              memcpy (session->security_parameters.extensions.
                      server_names[i].name, p, len);
              session->security_parameters.extensions.server_names[i].
                name_length = len;
              session->security_parameters.extensions.server_names[i].type =
                GNUTLS_NAME_DNS;
              break;
            }
        }

      /* move to next record */
      p += len;
    }
  return 0;
}

/* returns data_size or a negative number on failure
 */
int
MHD_gtls_server_name_send_params (MHD_gtls_session_t session,
                                  opaque * data, size_t _data_size)
{
  int total_size = 0;
#if MHD_DEBUG_TLS
  uint16_t len;
  opaque *p;
  unsigned i;
  ssize_t data_size = _data_size;

  /* this function sends the client extension data (dnsname) */
  if (session->security_parameters.entity == GNUTLS_CLIENT)
    {

      if (session->security_parameters.extensions.server_names_size == 0)
        return 0;

      /* uint16_t
       */
      total_size = 2;
      for (i = 0;
           i < session->security_parameters.extensions.server_names_size; i++)
        {
          /* count the total size
           */
          len =
            session->security_parameters.extensions.server_names[i].
            name_length;

          /* uint8_t + uint16_t + size
           */
          total_size += 1 + 2 + len;
        }

      p = data;

      /* UINT16: write total size of all names
       */
      DECR_LENGTH_RET (data_size, 2, GNUTLS_E_SHORT_MEMORY_BUFFER);
      MHD_gtls_write_uint16 (total_size - 2, p);
      p += 2;

      for (i = 0;
           i < session->security_parameters.extensions.server_names_size; i++)
        {

          switch (session->security_parameters.extensions.
                  server_names[i].type)
            {
            case GNUTLS_NAME_DNS:

              len =
                session->security_parameters.extensions.
                server_names[i].name_length;
              if (len == 0)
                break;

              /* UINT8: type of this extension
               * UINT16: size of the first name
               * LEN: the actual server name.
               */
              DECR_LENGTH_RET (data_size, len + 3,
                               GNUTLS_E_SHORT_MEMORY_BUFFER);

              *p = 0;           /* NAME_DNS type */
              p++;

              MHD_gtls_write_uint16 (len, p);
              p += 2;

              memcpy (p,
                      session->security_parameters.extensions.
                      server_names[0].name, len);
              p += len;
              break;
            default:
              MHD_gnutls_assert ();
              return GNUTLS_E_INTERNAL_ERROR;
            }
        }
    }
#endif
  return total_size;
}
