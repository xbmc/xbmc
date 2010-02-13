/*
 * Copyright (C) 2000, 2001, 2003, 2004, 2005 Free Software Foundation
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
#include <gnutls_datum.h>
#include <x509_b64.h>           /* for PKCS3 PEM decoding */
#include <gnutls_global.h>
#include <gnutls_dh.h>
#include "debug.h"
/* x509 */
#include "mpi.h"


/* returns the prime and the generator of DH params.
 */
const mpi_t *
MHD_gtls_dh_params_to_mpi (MHD_gtls_dh_params_t dh_primes)
{
  if (dh_primes == NULL || dh_primes->params[1] == NULL
      || dh_primes->params[0] == NULL)
    {
      return NULL;
    }

  return dh_primes->params;
}

int
MHD_gtls_dh_generate_prime (mpi_t * ret_g, mpi_t * ret_n, unsigned int bits)
{
  mpi_t g = NULL, prime = NULL;
  gcry_error_t err;
  int result, times = 0, qbits;
  mpi_t *factors = NULL;

  /* Calculate the size of a prime factor of (prime-1)/2.
   * This is an emulation of the values in "Selecting Cryptographic Key Sizes" paper.
   */
  if (bits < 256)
    qbits = bits / 2;
  else
    {
      qbits = (bits / 40) + 105;
    }

  if (qbits & 1)                /* better have an even number */
    qbits++;

  /* find a prime number of size bits.
   */
  do
    {

      if (times)
        {
          MHD_gtls_mpi_release (&prime);
          gcry_prime_release_factors (factors);
        }

      err = gcry_prime_generate (&prime, bits, qbits, &factors, NULL, NULL,
                                 GCRY_STRONG_RANDOM,
                                 GCRY_PRIME_FLAG_SPECIAL_FACTOR);

      if (err != 0)
        {
          MHD_gnutls_assert ();
          result = GNUTLS_E_INTERNAL_ERROR;
          goto cleanup;
        }

      err = gcry_prime_check (prime, 0);

      times++;
    }
  while (err != 0 && times < 10);

  if (err != 0)
    {
      MHD_gnutls_assert ();
      result = GNUTLS_E_INTERNAL_ERROR;
      goto cleanup;
    }

  /* generate the group generator.
   */
  err = gcry_prime_group_generator (&g, prime, factors, NULL);
  if (err != 0)
    {
      MHD_gnutls_assert ();
      result = GNUTLS_E_INTERNAL_ERROR;
      goto cleanup;
    }

  gcry_prime_release_factors (factors);
  factors = NULL;

  if (ret_g)
    *ret_g = g;
  else
    MHD_gtls_mpi_release (&g);
  if (ret_n)
    *ret_n = prime;
  else
    MHD_gtls_mpi_release (&prime);

  return 0;

cleanup:gcry_prime_release_factors (factors);
  MHD_gtls_mpi_release (&g);
  MHD_gtls_mpi_release (&prime);

  return result;

}

/* Replaces the prime in the static DH parameters, with a randomly
 * generated one.
 */
/**
 * MHD__gnutls_dh_params_init - This function will initialize the DH parameters
 * @dh_params: Is a structure that will hold the prime numbers
 *
 * This function will initialize the DH parameters structure.
 *
 **/
int
MHD__gnutls_dh_params_init (MHD_gtls_dh_params_t * dh_params)
{

  (*dh_params) = MHD_gnutls_calloc (1, sizeof (MHD_gtls_dh_params_st));
  if (*dh_params == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  return 0;

}

/**
 * MHD__gnutls_dh_params_deinit - This function will deinitialize the DH parameters
 * @dh_params: Is a structure that holds the prime numbers
 *
 * This function will deinitialize the DH parameters structure.
 *
 **/
void
MHD__gnutls_dh_params_deinit (MHD_gtls_dh_params_t dh_params)
{
  if (dh_params == NULL)
    return;

  MHD_gtls_mpi_release (&dh_params->params[0]);
  MHD_gtls_mpi_release (&dh_params->params[1]);

  MHD_gnutls_free (dh_params);

}

/**
 * MHD__gnutls_dh_params_generate2 - This function will generate new DH parameters
 * @params: Is the structure that the DH parameters will be stored
 * @bits: is the prime's number of bits
 *
 * This function will generate a new pair of prime and generator for use in
 * the Diffie-Hellman key exchange. The new parameters will be allocated using
 * MHD_gnutls_malloc() and will be stored in the appropriate datum.
 * This function is normally slow.
 *
 * Note that the bits value should be one of 768, 1024, 2048, 3072 or 4096.
 * Also note that the DH parameters are only useful to servers.
 * Since clients use the parameters sent by the server, it's of
 * no use to call this in client side.
 *
 **/
int
MHD__gnutls_dh_params_generate2 (MHD_gtls_dh_params_t params,
                                 unsigned int bits)
{
  int ret;

  ret =
    MHD_gtls_dh_generate_prime (&params->params[1], &params->params[0], bits);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  return 0;
}
