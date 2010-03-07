/*
 * Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2007 Free Software Foundation
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

/* This file contains the RSA key exchange part of the certificate
 * authentication.
 */

#include "gnutls_int.h"
#include "gnutls_auth_int.h"
#include "gnutls_errors.h"
#include "gnutls_dh.h"
#include "gnutls_num.h"
#include "gnutls_datum.h"
#include "auth_cert.h"
#include <gnutls_pk.h>
#include <gnutls_algorithms.h>
#include <gnutls_global.h>
#include "debug.h"
#include <gnutls_sig.h>
#include <gnutls_x509.h>
#include <gnutls_rsa_export.h>
#include <gnutls_state.h>

int MHD__gnutls_gen_rsa_client_kx (MHD_gtls_session_t, opaque **);
int MHD__gnutls_proc_rsa_client_kx (MHD_gtls_session_t, opaque *, size_t);
static int gen_rsa_export_server_kx (MHD_gtls_session_t, opaque **);
static int proc_rsa_export_server_kx (MHD_gtls_session_t, opaque *, size_t);

const MHD_gtls_mod_auth_st MHD_rsa_export_auth_struct = {
  "RSA EXPORT",
  MHD_gtls_gen_cert_server_certificate,
  MHD_gtls_gen_cert_client_certificate,
  gen_rsa_export_server_kx,
  MHD__gnutls_gen_rsa_client_kx,
  MHD_gtls_gen_cert_client_cert_vrfy,   /* gen client cert vrfy */
  MHD_gtls_gen_cert_server_cert_req,    /* server cert request */

  MHD_gtls_proc_cert_server_certificate,
  MHD__gnutls_proc_cert_client_certificate,
  proc_rsa_export_server_kx,
  MHD__gnutls_proc_rsa_client_kx,       /* proc client kx */
  MHD_gtls_proc_cert_client_cert_vrfy,  /* proc client cert vrfy */
  MHD_gtls_proc_cert_cert_req   /* proc server cert request */
};

static int
gen_rsa_export_server_kx (MHD_gtls_session_t session, opaque ** data)
{
  MHD_gtls_rsa_params_t rsa_params;
  const mpi_t *rsa_mpis;
  size_t n_e, n_m;
  uint8_t *data_e, *data_m;
  int ret = 0, data_size;
  MHD_gnutls_cert *apr_cert_list;
  MHD_gnutls_privkey *apr_pkey;
  int apr_cert_list_length;
  MHD_gnutls_datum_t signature, ddata;
  MHD_gtls_cert_credentials_t cred;

  cred = (MHD_gtls_cert_credentials_t)
    MHD_gtls_get_cred (session->key, MHD_GNUTLS_CRD_CERTIFICATE, NULL);
  if (cred == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
    }

  /* find the appropriate certificate */
  if ((ret =
       MHD_gtls_get_selected_cert (session, &apr_cert_list,
                                   &apr_cert_list_length, &apr_pkey)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  /* abort sending this message if we have a certificate
   * of 512 bits or less.
   */
  if (apr_pkey && MHD__gnutls_mpi_get_nbits (apr_pkey->params[0]) <= 512)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INT_RET_0;
    }

  rsa_params =
    MHD_gtls_certificate_get_rsa_params (cred->rsa_params, cred->params_func,
                                         session);
  rsa_mpis = MHD__gnutls_rsa_params_to_mpi (rsa_params);
  if (rsa_mpis == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_NO_TEMPORARY_RSA_PARAMS;
    }

  if ((ret = MHD_gtls_auth_info_set (session, MHD_GNUTLS_CRD_CERTIFICATE,
                                     sizeof (cert_auth_info_st), 0)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  MHD_gtls_rsa_export_set_pubkey (session, rsa_mpis[1], rsa_mpis[0]);

  MHD_gtls_mpi_print (NULL, &n_m, rsa_mpis[0]);
  MHD_gtls_mpi_print (NULL, &n_e, rsa_mpis[1]);

  (*data) = MHD_gnutls_malloc (n_e + n_m + 4);
  if (*data == NULL)
    {
      return GNUTLS_E_MEMORY_ERROR;
    }

  data_m = &(*data)[0];
  MHD_gtls_mpi_print (&data_m[2], &n_m, rsa_mpis[0]);

  MHD_gtls_write_uint16 (n_m, data_m);

  data_e = &data_m[2 + n_m];
  MHD_gtls_mpi_print (&data_e[2], &n_e, rsa_mpis[1]);

  MHD_gtls_write_uint16 (n_e, data_e);

  data_size = n_m + n_e + 4;


  /* Generate the signature. */

  ddata.data = *data;
  ddata.size = data_size;

  if (apr_cert_list_length > 0)
    {
      if ((ret =
           MHD_gtls_tls_sign_params (session, &apr_cert_list[0],
                                     apr_pkey, &ddata, &signature)) < 0)
        {
          MHD_gnutls_assert ();
          MHD_gnutls_free (*data);
          *data = NULL;
          return ret;
        }
    }
  else
    {
      MHD_gnutls_assert ();
      return data_size;         /* do not put a signature - ILLEGAL! */
    }

  *data = MHD_gtls_realloc_fast (*data, data_size + signature.size + 2);
  if (*data == NULL)
    {
      MHD__gnutls_free_datum (&signature);
      MHD_gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  MHD_gtls_write_datum16 (&((*data)[data_size]), signature);
  data_size += signature.size + 2;

  MHD__gnutls_free_datum (&signature);

  return data_size;
}

/* if the peer's certificate is of 512 bits or less, returns non zero.
 */
int
MHD__gnutls_peers_cert_less_512 (MHD_gtls_session_t session)
{
  MHD_gnutls_cert peer_cert;
  int ret;
  cert_auth_info_t info = MHD_gtls_get_auth_info (session);

  if (info == NULL || info->ncerts == 0)
    {
      MHD_gnutls_assert ();
      /* we need this in order to get peer's certificate */
      return 0;
    }

  if ((ret =
       MHD_gtls_raw_cert_to_gcert (&peer_cert,
                                   session->security_parameters.cert_type,
                                   &info->raw_certificate_list[0],
                                   CERT_NO_COPY)) < 0)
    {
      MHD_gnutls_assert ();
      return 0;
    }

  if (peer_cert.subject_pk_algorithm != MHD_GNUTLS_PK_RSA)
    {
      MHD_gnutls_assert ();
      MHD_gtls_gcert_deinit (&peer_cert);
      return 0;
    }

  if (MHD__gnutls_mpi_get_nbits (peer_cert.params[0]) <= 512)
    {
      MHD_gtls_gcert_deinit (&peer_cert);
      return 1;
    }

  MHD_gtls_gcert_deinit (&peer_cert);

  return 0;
}

static int
proc_rsa_export_server_kx (MHD_gtls_session_t session,
                           opaque * data, size_t _data_size)
{
  uint16_t n_m, n_e;
  size_t _n_m, _n_e;
  uint8_t *data_m;
  uint8_t *data_e;
  int i, sigsize;
  MHD_gnutls_datum_t vparams, signature;
  int ret;
  ssize_t data_size = _data_size;
  cert_auth_info_t info;
  MHD_gnutls_cert peer_cert;

  info = MHD_gtls_get_auth_info (session);
  if (info == NULL || info->ncerts == 0)
    {
      MHD_gnutls_assert ();
      /* we need this in order to get peer's certificate */
      return GNUTLS_E_INTERNAL_ERROR;
    }


  i = 0;

  DECR_LEN (data_size, 2);
  n_m = MHD_gtls_read_uint16 (&data[i]);
  i += 2;

  DECR_LEN (data_size, n_m);
  data_m = &data[i];
  i += n_m;

  DECR_LEN (data_size, 2);
  n_e = MHD_gtls_read_uint16 (&data[i]);
  i += 2;

  DECR_LEN (data_size, n_e);
  data_e = &data[i];
  i += n_e;

  _n_e = n_e;
  _n_m = n_m;

  if (MHD_gtls_mpi_scan_nz (&session->key->rsa[0], data_m, &_n_m) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MPI_SCAN_FAILED;
    }

  if (MHD_gtls_mpi_scan_nz (&session->key->rsa[1], data_e, &_n_e) != 0)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_MPI_SCAN_FAILED;
    }

  MHD_gtls_rsa_export_set_pubkey (session, session->key->rsa[1],
                                  session->key->rsa[0]);

  /* VERIFY SIGNATURE */

  vparams.size = n_m + n_e + 4;
  vparams.data = data;

  DECR_LEN (data_size, 2);
  sigsize = MHD_gtls_read_uint16 (&data[vparams.size]);

  DECR_LEN (data_size, sigsize);
  signature.data = &data[vparams.size + 2];
  signature.size = sigsize;

  if ((ret =
       MHD_gtls_raw_cert_to_gcert (&peer_cert,
                                   session->security_parameters.cert_type,
                                   &info->raw_certificate_list[0],
                                   CERT_NO_COPY)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  ret =
    MHD_gtls_verify_sig_params (session, &peer_cert, &vparams, &signature);

  MHD_gtls_gcert_deinit (&peer_cert);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
    }

  return ret;
}
