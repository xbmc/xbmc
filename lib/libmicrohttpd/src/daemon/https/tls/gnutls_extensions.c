/*
 * Copyright (C) 2001, 2002, 2003, 2004, 2005, 2007 Free Software Foundation
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

/* Functions that relate to the TLS hello extension parsing.
 * Hello extensions are packets appended in the TLS hello packet, and
 * allow for extra functionality.
 */

#include "MHD_config.h"
#include "gnutls_int.h"
#include "gnutls_extensions.h"
#include "gnutls_errors.h"
#include "ext_max_record.h"
#include <ext_cert_type.h>
#include <ext_server_name.h>
#include <gnutls_num.h>

/* Key Exchange Section */
#define GNUTLS_EXTENSION_ENTRY(type, parse_type, ext_func_recv, ext_func_send) \
	{ #type, type, parse_type, ext_func_recv, ext_func_send }


#define MAX_EXT_SIZE 10
const int MHD_gtls_extensions_size = MAX_EXT_SIZE;

MHD_gtls_extension_entry MHD_gtls_extensions[MAX_EXT_SIZE] = {
  GNUTLS_EXTENSION_ENTRY (GNUTLS_EXTENSION_MAX_RECORD_SIZE,
                          EXTENSION_TLS,
                          MHD_gtls_max_record_recv_params,
                          MHD_gtls_max_record_send_params),
  GNUTLS_EXTENSION_ENTRY (GNUTLS_EXTENSION_CERT_TYPE,
                          EXTENSION_TLS,
                          MHD_gtls_cert_type_recv_params,
                          MHD_gtls_cert_type_send_params),
  GNUTLS_EXTENSION_ENTRY (GNUTLS_EXTENSION_SERVER_NAME,
                          EXTENSION_APPLICATION,
                          MHD_gtls_server_name_recv_params,
                          MHD_gtls_server_name_send_params),
  {0, 0, 0, 0}
};

#define GNUTLS_EXTENSION_LOOP2(b) \
        MHD_gtls_extension_entry *p; \
                for(p = MHD_gtls_extensions; p->name != NULL; p++) { b ; }

#define GNUTLS_EXTENSION_LOOP(a) \
                        GNUTLS_EXTENSION_LOOP2( if(p->type == type) { a; break; } )


/* EXTENSION functions */

MHD_gtls_ext_recv_func
MHD_gtls_ext_func_recv (uint16_t type, MHD_gtls_ext_parse_type_t parse_type)
{
  MHD_gtls_ext_recv_func ret = NULL;
  GNUTLS_EXTENSION_LOOP (if
                         (parse_type == EXTENSION_ANY
                          || p->parse_type == parse_type) ret =
                         p->MHD_gnutls_ext_func_recv);
  return ret;

}

MHD_gtls_ext_send_func
MHD_gtls_ext_func_send (uint16_t type)
{
  MHD_gtls_ext_send_func ret = NULL;
  GNUTLS_EXTENSION_LOOP (ret = p->MHD_gnutls_ext_func_send);
  return ret;

}

const char *
MHD_gtls_extension_get_name (uint16_t type)
{
  const char *ret = NULL;

  /* avoid prefix */
  GNUTLS_EXTENSION_LOOP (ret = p->name + sizeof ("GNUTLS_EXTENSION_") - 1);

  return ret;
}

/* Checks if the extension we just received is one of the
 * requested ones. Otherwise it's a fatal error.
 */
static int
MHD__gnutls_extension_list_check (MHD_gtls_session_t session, uint16_t type)
{
#if MHD_DEBUG_TLS
  if (session->security_parameters.entity == GNUTLS_CLIENT)
    {
      int i;
      for (i = 0; i < session->internals.extensions_sent_size; i++)
        {
          if (type == session->internals.extensions_sent[i])
            return 0;           /* ok found */
        }
      return GNUTLS_E_RECEIVED_ILLEGAL_EXTENSION;
    }
#endif
  return 0;
}

int
MHD_gtls_parse_extensions (MHD_gtls_session_t session,
                           MHD_gtls_ext_parse_type_t parse_type,
                           const opaque * data, int data_size)
{
  int next, ret;
  int pos = 0;
  uint16_t type;
  const opaque *sdata;
  MHD_gtls_ext_recv_func ext_recv;
  uint16_t size;

#if MHD_DEBUG_TLS
  int i;
  if (session->security_parameters.entity == GNUTLS_CLIENT)
    for (i = 0; i < session->internals.extensions_sent_size; i++)
      {
        MHD__gnutls_debug_log ("EXT[%d]: expecting extension '%s'\n",
                               session,
                               MHD_gtls_extension_get_name
                               (session->internals.extensions_sent[i]));
      }
#endif

  DECR_LENGTH_RET (data_size, 2, 0);
  next = MHD_gtls_read_uint16 (data);
  pos += 2;

  DECR_LENGTH_RET (data_size, next, 0);

  do
    {
      DECR_LENGTH_RET (next, 2, 0);
      type = MHD_gtls_read_uint16 (&data[pos]);
      pos += 2;

      MHD__gnutls_debug_log ("EXT[%x]: Received extension '%s/%d'\n", session,
                             MHD_gtls_extension_get_name (type), type);

      if ((ret = MHD__gnutls_extension_list_check (session, type)) < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

      DECR_LENGTH_RET (next, 2, 0);
      size = MHD_gtls_read_uint16 (&data[pos]);
      pos += 2;

      DECR_LENGTH_RET (next, size, 0);
      sdata = &data[pos];
      pos += size;

      ext_recv = MHD_gtls_ext_func_recv (type, parse_type);
      if (ext_recv == NULL)
        continue;
      if ((ret = ext_recv (session, sdata, size)) < 0)
        {
          MHD_gnutls_assert ();
          return ret;
        }

    }
  while (next > 2);

  return 0;

}

/* Adds the extension we want to send in the extensions list.
 * This list is used to check whether the (later) received
 * extensions are the ones we requested.
 */
static void
MHD__gnutls_extension_list_add (MHD_gtls_session_t session, uint16_t type)
{
#if MHD_DEBUG_TLS
  if (session->security_parameters.entity == GNUTLS_CLIENT)
    {
      if (session->internals.extensions_sent_size < MAX_EXT_TYPES)
        {
          session->internals.extensions_sent[session->internals.
                                             extensions_sent_size] = type;
          session->internals.extensions_sent_size++;
        }
      else
        {
          MHD__gnutls_debug_log ("extensions: Increase MAX_EXT_TYPES\n");
        }
    }
#endif
}

int
MHD_gtls_gen_extensions (MHD_gtls_session_t session, opaque * data,
                         size_t data_size)
{
  int size;
  uint16_t pos = 0;
  opaque *sdata;
  int sdata_size;
  MHD_gtls_ext_send_func ext_send;
  MHD_gtls_extension_entry *p;

  if (data_size < 2)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  /* allocate enough data for each extension.
   */
  sdata_size = data_size;
  sdata = MHD_gnutls_malloc (sdata_size);
  if (sdata == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  pos += 2;
  for (p = MHD_gtls_extensions; p->name != NULL; p++)
    {
      ext_send = MHD_gtls_ext_func_send (p->type);
      if (ext_send == NULL)
        continue;
      size = ext_send (session, sdata, sdata_size);
      if (size > 0)
        {
          if (data_size < pos + (size_t) size + 4)
            {
              MHD_gnutls_assert ();
              MHD_gnutls_free (sdata);
              return GNUTLS_E_INTERNAL_ERROR;
            }

          /* write extension type */
          MHD_gtls_write_uint16 (p->type, &data[pos]);
          pos += 2;

          /* write size */
          MHD_gtls_write_uint16 (size, &data[pos]);
          pos += 2;

          memcpy (&data[pos], sdata, size);
          pos += size;

          /* add this extension to the extension list
           */
          MHD__gnutls_extension_list_add (session, p->type);

          MHD__gnutls_debug_log ("EXT[%x]: Sending extension %s\n", session,
                                 MHD_gtls_extension_get_name (p->type));
        }
      else if (size < 0)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_free (sdata);
          return size;
        }
    }

  size = pos;
  pos -= 2;                     /* remove the size of the size header! */

  MHD_gtls_write_uint16 (pos, data);

  if (size == 2)
    {                           /* empty */
      size = 0;
    }

  MHD_gnutls_free (sdata);
  return size;

}
