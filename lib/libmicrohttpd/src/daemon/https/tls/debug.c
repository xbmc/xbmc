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

#include "gnutls_int.h"
#include "gnutls_errors.h"
#include <stdio.h>
#include <stdlib.h>
#include <gcrypt.h>

const char *
MHD__gnutls_packet2str (content_type_t packet)
{
  switch (packet)
    {
    case GNUTLS_CHANGE_CIPHER_SPEC:
      return "Change Cipher Spec";
    case GNUTLS_ALERT:
      return "Alert";
    case GNUTLS_HANDSHAKE:
      return "Handshake";
    case GNUTLS_APPLICATION_DATA:
      return "Application Data";
    case GNUTLS_INNER_APPLICATION:
      return "Inner Application";

    default:
      return "Unknown Packet";
    }
}

const char *
MHD__gnutls_handshake2str (MHD_gnutls_handshake_description_t handshake)
{

  switch (handshake)
    {
    case GNUTLS_HANDSHAKE_HELLO_REQUEST:
      return "HELLO REQUEST";
      break;
    case GNUTLS_HANDSHAKE_CLIENT_HELLO:
      return "CLIENT HELLO";
      break;
    case GNUTLS_HANDSHAKE_SERVER_HELLO:
      return "SERVER HELLO";
      break;
    case GNUTLS_HANDSHAKE_CERTIFICATE_PKT:
      return "CERTIFICATE";
      break;
    case GNUTLS_HANDSHAKE_SERVER_KEY_EXCHANGE:
      return "SERVER KEY EXCHANGE";
      break;
    case GNUTLS_HANDSHAKE_CERTIFICATE_REQUEST:
      return "CERTIFICATE REQUEST";
      break;
    case GNUTLS_HANDSHAKE_SERVER_HELLO_DONE:
      return "SERVER HELLO DONE";
      break;
    case GNUTLS_HANDSHAKE_CERTIFICATE_VERIFY:
      return "CERTIFICATE VERIFY";
      break;
    case GNUTLS_HANDSHAKE_CLIENT_KEY_EXCHANGE:
      return "CLIENT KEY EXCHANGE";
      break;
    case GNUTLS_HANDSHAKE_FINISHED:
      return "FINISHED";
      break;
    case GNUTLS_HANDSHAKE_SUPPLEMENTAL:
      return "SUPPLEMENTAL";
      break;
    default:
      return "Unknown Handshake packet";

    }
}
