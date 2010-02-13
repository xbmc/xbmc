/*
 * Copyright (C) 2004, 2005, 2006, 2007 Free Software Foundation
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

/* Here lies the code of the MHD_gnutls_*_set_priority() functions.
 */

#include "gnutls_int.h"
#include "gnutls_algorithms.h"
#include "gnutls_errors.h"
#include <gnutls_num.h>

#define MAX_ELEMENTS 48

static int
_set_priority (MHD_gtls_priority_st * st, const int *list)
{
  int num = 0;

  while ((list[num] != 0) && (num < MAX_ALGOS))
    num++;
  st->num_algorithms = num;
  memcpy (st->priority, list, num * sizeof (int));
  return 0;
}

static const int MHD_gtls_protocol_priority[] = { MHD_GNUTLS_PROTOCOL_TLS1_1,
  MHD_GNUTLS_PROTOCOL_TLS1_0,
  MHD_GNUTLS_PROTOCOL_SSL3,
  0
};

static const int MHD_gtls_cipher_priority_secure256[] =
  { MHD_GNUTLS_CIPHER_AES_256_CBC,
  0
};

static const int MHD_gtls_kx_priority_secure[] = { MHD_GNUTLS_KX_RSA,
  0
};

static const int MHD_gtls_mac_priority_secure[] = { MHD_GNUTLS_MAC_SHA1,
  0
};

static int MHD_gtls_cert_type_priority[] = { MHD_GNUTLS_CRT_X509,
  0
};

static const int MHD_gtls_comp_priority[] = { MHD_GNUTLS_COMP_NULL,
  0
};

/**
 * MHD__gnutls_priority_set - Sets priorities for the cipher suites supported by gnutls.
 * @session: is a #MHD_gtls_session_t structure.
 * @priority: is a #MHD_gnutls_priority_t structure.
 *
 * Sets the priorities to use on the ciphers, key exchange methods,
 * macs and compression methods.
 *
 * On success 0 is returned.
 *
 **/
int
MHD__gnutls_priority_set (MHD_gtls_session_t session,
                          MHD_gnutls_priority_t priority)
{
  if (priority == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_NO_CIPHER_SUITES;
    }

  memcpy (&session->internals.priorities, priority,
          sizeof (struct MHD_gtls_priority_st));

  return 0;
}

/**
 * MHD_tls_set_default_priority - Sets priorities for the cipher suites supported by gnutls.
 * @priority_cache: is a #MHD_gnutls_prioritity_t structure.
 * @priorities: is a string describing priorities
 * @err_pos: In case of an error this will have the position in the string the error occured
 *
 * Sets priorities for the ciphers, key exchange methods, macs and
 * compression methods. This is to avoid using the
 * MHD_gnutls_*_priority() functions.
 *
 * The #priorities option allows you to specify a semi-colon
 * separated list of the cipher priorities to enable.
 *
 * Unless the first keyword is "NONE" the defaults are:
 * Protocols: TLS1.1, TLS1.0, and SSL3.0.
 * Compression: NULL.
 * Certificate types: X.509, OpenPGP.
 *
 * You can also use predefined sets of ciphersuites: "PERFORMANCE"
 * all the "secure" ciphersuites are enabled, limited to 128 bit
 * ciphers and sorted by terms of speed performance.
 *
 * "NORMAL" option enables all "secure" ciphersuites. The 256-bit ciphers
 * are included as a fallback only. The ciphers are sorted by security margin.
 *
 * "SECURE128" flag enables all "secure" ciphersuites with ciphers up to
 * 128 bits, sorted by security margin.
 *
 * "SECURE256" flag enables all "secure" ciphersuites including the 256 bit
 * ciphers, sorted by security margin.
 *
 * "EXPORT" all the ciphersuites are enabled, including the
 * low-security 40 bit ciphers.
 *
 * "NONE" nothing is enabled. This disables even protocols and
 * compression methods.
 *
 * Special keywords:
 * '!' or '-' appended with an algorithm will remove this algorithm.
 * '+' appended with an algorithm will add this algorithm.
 * '%COMPAT' will enable compatibility features for a server.
 *
 * To avoid collisions in order to specify a compression algorithm in
 * this string you have to prefix it with "COMP-", protocol versions
 * with "VERS-" and certificate types with "CTYPE-". All other
 * algorithms don't need a prefix.
 *
 * For key exchange algorithms when in NORMAL or SECURE levels the
 * perfect forward secrecy algorithms take precendence of the other
 * protocols.  In all cases all the supported key exchange algorithms
 * are enabled (except for the RSA-EXPORT which is only enabled in
 * EXPORT level).
 *
 * Note that although one can select very long key sizes (such as 256 bits)
 * for symmetric algorithms, to actually increase security the public key
 * algorithms have to use longer key sizes as well.
 *
 * Examples: "NORMAL:!AES-128-CBC",
 * "EXPORT:!VERS-TLS1.0:+COMP-DEFLATE:+CTYPE-OPENPGP",
 * "NONE:+VERS-TLS1.0:+AES-128-CBC:+RSA:+SHA1:+COMP-NULL", "NORMAL",
 * "NORMAL:%COMPAT".
 *
 * Returns: On syntax error GNUTLS_E_INVALID_REQUEST is returned and
 * 0 on success.
 **/
int
MHD_tls_set_default_priority (MHD_gnutls_priority_t * priority_cache,
                              const char *priorities, const char **err_pos)
{
  *priority_cache =
    MHD_gnutls_calloc (1, sizeof (struct MHD_gtls_priority_st));
  if (*priority_cache == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  /* set mode to "SECURE256" */
  _set_priority (&(*priority_cache)->protocol, MHD_gtls_protocol_priority);
  _set_priority (&(*priority_cache)->cipher,
                 MHD_gtls_cipher_priority_secure256);
  _set_priority (&(*priority_cache)->kx, MHD_gtls_kx_priority_secure);
  _set_priority (&(*priority_cache)->mac, MHD_gtls_mac_priority_secure);
  _set_priority (&(*priority_cache)->cert_type, MHD_gtls_cert_type_priority);
  _set_priority (&(*priority_cache)->compression, MHD_gtls_comp_priority);

  (*priority_cache)->no_padding = 0;
  return 0;
}

/**
 * MHD__gnutls_priority_deinit - Deinitialize the priorities cache for the cipher suites supported by gnutls.
 * @priority_cache: is a #MHD_gnutls_prioritity_t structure.
 *
 * Deinitializes the priority cache.
 *
 **/
void
MHD__gnutls_priority_deinit (MHD_gnutls_priority_t priority_cache)
{
  MHD_gnutls_free (priority_cache);
}

/**
 * MHD__gnutls_priority_set_direct - Sets priorities for the cipher suites supported by gnutls.
 * @session: is a #MHD_gtls_session_t structure.
 * @priorities: is a string describing priorities
 * @err_pos: In case of an error this will have the position in the string the error occured
 *
 * Sets the priorities to use on the ciphers, key exchange methods,
 * macs and compression methods. This function avoids keeping a
 * priority cache and is used to directly set string priorities to a
 * TLS session.  For documentation check the MHD_tls_set_default_priority().
 *
 * On syntax error GNUTLS_E_INVALID_REQUEST is returned and 0 on success.
 *
 **/
int
MHD__gnutls_priority_set_direct (MHD_gtls_session_t session,
                                 const char *priorities, const char **err_pos)
{
  MHD_gnutls_priority_t prio;
  int ret;

  ret = MHD_tls_set_default_priority (&prio, priorities, err_pos);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  ret = MHD__gnutls_priority_set (session, prio);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  MHD__gnutls_priority_deinit (prio);

  return 0;
}
