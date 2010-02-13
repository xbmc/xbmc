/*
 * Copyright (C) 2002, 2003, 2004, 2005, 2007 Free Software Foundation
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

/* This file contains common stuff in Ephemeral Diffie Hellman (DHE) and
 * Anonymous DH key exchange(DHA). These are used in the handshake procedure
 * of the certificate and anoymous authentication.
 */

#include "gnutls_int.h"
#include "gnutls_auth_int.h"
#include "gnutls_errors.h"
#include "gnutls_dh.h"
#include "gnutls_num.h"
#include "gnutls_sig.h"
#include <gnutls_datum.h>
#include <gnutls_x509.h>
#include <gnutls_state.h>
#include <auth_dh_common.h>
#include <gnutls_algorithms.h>

/* Frees the MHD_gtls_dh_info_st structure.
 */
void
MHD_gtls_free_dh_info (MHD_gtls_dh_info_st * dh)
{
  dh->secret_bits = 0;
  MHD__gnutls_free_datum (&dh->prime);
  MHD__gnutls_free_datum (&dh->generator);
  MHD__gnutls_free_datum (&dh->public_key);
}

int
MHD_gtls_proc_dh_common_client_kx (MHD_gtls_session_t session,
                                   opaque * data, size_t _data_size,
                                   mpi_t g, mpi_t p)
{
  uint16_t n_Y;
  size_t _n_Y;
  int ret;
  ssize_t data_size = _data_size;


  DECR_LEN (data_size, 2);
  n_Y = MHD_gtls_read_uint16 (&data[0]);
  _n_Y = n_Y;

  DECR_LEN (data_size, n_Y);
  if (MHD_gtls_mpi_scan_nz (&session->key->client_Y, &data[2], &_n_Y))
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MPI_SCAN_FAILED;
    }

  MHD_gtls_dh_set_peer_public (session, session->key->client_Y);

  session->key->KEY =
    MHD_gtls_calc_dh_key (session->key->client_Y, session->key->dh_secret, p);

  if (session->key->KEY == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  MHD_gtls_mpi_release (&session->key->client_Y);
  MHD_gtls_mpi_release (&session->key->dh_secret);

  ret = MHD_gtls_mpi_dprint (&session->key->key, session->key->KEY);

  MHD_gtls_mpi_release (&session->key->KEY);

  if (ret < 0)
    {
      return ret;
    }

  return 0;
}

int
MHD_gtls_gen_dh_common_client_kx (MHD_gtls_session_t session, opaque ** data)
{
  mpi_t x = NULL, X = NULL;
  size_t n_X;
  int ret;

  *data = NULL;

  X = MHD_gtls_calc_dh_secret (&x, session->key->client_g,
                               session->key->client_p);
  if (X == NULL || x == NULL)
    {
      MHD_gnutls_assert ();
      ret = GNUTLS_E_MEMORY_ERROR;
      goto error;
    }

  MHD_gtls_dh_set_secret_bits (session, MHD__gnutls_mpi_get_nbits (x));

  MHD_gtls_mpi_print (NULL, &n_X, X);
  (*data) = MHD_gnutls_malloc (n_X + 2);
  if (*data == NULL)
    {
      ret = GNUTLS_E_MEMORY_ERROR;
      goto error;
    }

  MHD_gtls_mpi_print (&(*data)[2], &n_X, X);
  MHD_gtls_mpi_release (&X);

  MHD_gtls_write_uint16 (n_X, &(*data)[0]);

  /* calculate the key after calculating the message */
  session->key->KEY =
    MHD_gtls_calc_dh_key (session->key->client_Y, x, session->key->client_p);

  MHD_gtls_mpi_release (&x);
  if (session->key->KEY == NULL)
    {
      MHD_gnutls_assert ();
      ret = GNUTLS_E_MEMORY_ERROR;
      goto error;
    }

  /* THESE SHOULD BE DISCARDED */
  MHD_gtls_mpi_release (&session->key->client_Y);
  MHD_gtls_mpi_release (&session->key->client_p);
  MHD_gtls_mpi_release (&session->key->client_g);

  ret = MHD_gtls_mpi_dprint (&session->key->key, session->key->KEY);

  MHD_gtls_mpi_release (&session->key->KEY);

  if (ret < 0)
    {
      MHD_gnutls_assert ();
      goto error;
    }

  return n_X + 2;

error:
  MHD_gtls_mpi_release (&x);
  MHD_gtls_mpi_release (&X);
  MHD_gnutls_free (*data);
  *data = NULL;
  return ret;
}

int
MHD_gtls_proc_dh_common_server_kx (MHD_gtls_session_t session,
                                   opaque * data, size_t _data_size, int psk)
{
  uint16_t n_Y, n_g, n_p;
  size_t _n_Y, _n_g, _n_p;
  uint8_t *data_p;
  uint8_t *data_g;
  uint8_t *data_Y;
  int i, bits, psk_size, ret;
  ssize_t data_size = _data_size;

  i = 0;

  if (psk != 0)
    {
      DECR_LEN (data_size, 2);
      psk_size = MHD_gtls_read_uint16 (&data[i]);
      DECR_LEN (data_size, psk_size);
      i += 2 + psk_size;
    }

  DECR_LEN (data_size, 2);
  n_p = MHD_gtls_read_uint16 (&data[i]);
  i += 2;

  DECR_LEN (data_size, n_p);
  data_p = &data[i];
  i += n_p;

  DECR_LEN (data_size, 2);
  n_g = MHD_gtls_read_uint16 (&data[i]);
  i += 2;

  DECR_LEN (data_size, n_g);
  data_g = &data[i];
  i += n_g;

  DECR_LEN (data_size, 2);
  n_Y = MHD_gtls_read_uint16 (&data[i]);
  i += 2;

  DECR_LEN (data_size, n_Y);
  data_Y = &data[i];
  i += n_Y;

  _n_Y = n_Y;
  _n_g = n_g;
  _n_p = n_p;

  if (MHD_gtls_mpi_scan_nz (&session->key->client_Y, data_Y, &_n_Y) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MPI_SCAN_FAILED;
    }

  if (MHD_gtls_mpi_scan_nz (&session->key->client_g, data_g, &_n_g) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MPI_SCAN_FAILED;
    }
  if (MHD_gtls_mpi_scan_nz (&session->key->client_p, data_p, &_n_p) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MPI_SCAN_FAILED;
    }

  bits = MHD_gtls_dh_get_allowed_prime_bits (session);
  if (bits < 0)
    {
      MHD_gnutls_assert ();
      return bits;
    }

  if (MHD__gnutls_mpi_get_nbits (session->key->client_p) < (size_t) bits)
    {
      /* the prime used by the peer is not acceptable
       */
      MHD_gnutls_assert ();
      return GNUTLS_E_DH_PRIME_UNACCEPTABLE;
    }

  MHD_gtls_dh_set_group (session, session->key->client_g,
                         session->key->client_p);
  MHD_gtls_dh_set_peer_public (session, session->key->client_Y);

  ret = n_Y + n_p + n_g + 6;
  if (psk != 0)
    ret += 2;

  return ret;
}

/* If the psk flag is set, then an empty psk_identity_hint will
 * be inserted */
int
MHD_gtls_dh_common_print_server_kx (MHD_gtls_session_t session,
                                    mpi_t g, mpi_t p, opaque ** data, int psk)
{
  mpi_t x, X;
  size_t n_X, n_g, n_p;
  int ret, data_size, pos;
  uint8_t *pdata;

  X = MHD_gtls_calc_dh_secret (&x, g, p);
  if (X == NULL || x == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  session->key->dh_secret = x;
  MHD_gtls_dh_set_secret_bits (session, MHD__gnutls_mpi_get_nbits (x));

  MHD_gtls_mpi_print (NULL, &n_g, g);
  MHD_gtls_mpi_print (NULL, &n_p, p);
  MHD_gtls_mpi_print (NULL, &n_X, X);

  data_size = n_g + n_p + n_X + 6;
  if (psk != 0)
    data_size += 2;

  (*data) = MHD_gnutls_malloc (data_size);
  if (*data == NULL)
    {
      MHD_gtls_mpi_release (&X);
      return GNUTLS_E_MEMORY_ERROR;
    }

  pos = 0;
  pdata = *data;

  if (psk != 0)
    {
      MHD_gtls_write_uint16 (0, &pdata[pos]);
      pos += 2;
    }

  MHD_gtls_mpi_print (&pdata[pos + 2], &n_p, p);
  MHD_gtls_write_uint16 (n_p, &pdata[pos]);

  pos += n_p + 2;

  MHD_gtls_mpi_print (&pdata[pos + 2], &n_g, g);
  MHD_gtls_write_uint16 (n_g, &pdata[pos]);

  pos += n_g + 2;

  MHD_gtls_mpi_print (&pdata[pos + 2], &n_X, X);
  MHD_gtls_mpi_release (&X);

  MHD_gtls_write_uint16 (n_X, &pdata[pos]);

  ret = data_size;

  return ret;
}
