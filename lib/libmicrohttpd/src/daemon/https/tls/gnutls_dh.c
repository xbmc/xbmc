/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005 Free Software Foundation
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

#include <gnutls_int.h>
#include <gnutls_errors.h>


/*
	--Example--
	you: X = g ^ x mod p;
	peer:Y = g ^ y mod p;

	your_key = Y ^ x mod p;
	his_key  = X ^ y mod p;

//      generate our secret and the public value (X) for it
	X = MHD_gtls_calc_dh_secret(&x, g, p);
//      now we can calculate the shared secret
	key = MHD_gtls_calc_dh_key(Y, x, g, p);
	MHD_gtls_mpi_release(x);
	MHD_gtls_mpi_release(g);
*/

#define MAX_BITS 18000

/* returns the public value (X), and the secret (ret_x).
 */
mpi_t
MHD_gtls_calc_dh_secret (mpi_t * ret_x, mpi_t g, mpi_t prime)
{
  mpi_t e, x;
  int x_size = MHD__gnutls_mpi_get_nbits (prime) - 1;
  /* The size of the secret key is less than
   * prime/2
   */

  if (x_size > MAX_BITS || x_size <= 0)
    {
      MHD_gnutls_assert ();
      return NULL;
    }

  x = MHD__gnutls_mpi_new (x_size);
  if (x == NULL)
    {
      MHD_gnutls_assert ();
      if (ret_x)
        *ret_x = NULL;

      return NULL;
    }

  /* FIXME: (x_size/8)*8 is there to overcome a bug in libgcrypt
   * which does not really check the bits given but the bytes.
   */
  do
    {
      MHD__gnutls_mpi_randomize (x, (x_size / 8) * 8, GCRY_STRONG_RANDOM);
      /* Check whether x is zero.
       */
    }
  while (MHD__gnutls_mpi_cmp_ui (x, 0) == 0);

  e = MHD__gnutls_mpi_alloc_like (prime);
  if (e == NULL)
    {
      MHD_gnutls_assert ();
      if (ret_x)
        *ret_x = NULL;

      MHD_gtls_mpi_release (&x);
      return NULL;
    }

  MHD__gnutls_mpi_powm (e, g, x, prime);

  if (ret_x)
    *ret_x = x;
  else
    MHD_gtls_mpi_release (&x);
  return e;
}


mpi_t
MHD_gtls_calc_dh_key (mpi_t f, mpi_t x, mpi_t prime)
{
  mpi_t k;
  int bits;

  bits = MHD__gnutls_mpi_get_nbits (prime);
  if (bits <= 0 || bits > MAX_BITS)
    {
      MHD_gnutls_assert ();
      return NULL;
    }

  k = MHD__gnutls_mpi_alloc_like (prime);
  if (k == NULL)
    return NULL;
  MHD__gnutls_mpi_powm (k, f, x, prime);
  return k;
}

/*-
  * MHD_gtls_get_dh_params - Returns the DH parameters pointer
  * @dh_params: is an DH parameters structure, or NULL.
  * @func: is a callback function to receive the parameters or NULL.
  * @session: a gnutls session.
  *
  * This function will return the dh parameters pointer.
  *
  -*/
MHD_gtls_dh_params_t
MHD_gtls_get_dh_params (MHD_gtls_dh_params_t dh_params,
                        MHD_gnutls_params_function * func,
                        MHD_gtls_session_t session)
{
  MHD_gnutls_params_st params;
  int ret;

  /* if cached return the cached */
  if (session->internals.params.dh_params)
    return session->internals.params.dh_params;

  if (dh_params)
    {
      session->internals.params.dh_params = dh_params;
    }
  else if (func)
    {
      ret = func (session, GNUTLS_PARAMS_DH, &params);
      if (ret == 0 && params.type == GNUTLS_PARAMS_DH)
        {
          session->internals.params.dh_params = params.params.dh;
          session->internals.params.free_dh_params = params.deinit;
        }
    }

  return session->internals.params.dh_params;
}
