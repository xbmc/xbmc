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

/* This file contains everything for the Ephemeral Diffie Hellman (DHE)
 * key exchange. This is used in the handshake procedure of the certificate
 * authentication.
 */

#include "gnutls_int.h"
#include "gnutls_auth_int.h"
#include "gnutls_errors.h"
#include "gnutls_dh.h"
#include "gnutls_num.h"
#include "gnutls_sig.h"
#include <gnutls_datum.h>
#include <auth_cert.h>
#include <gnutls_x509.h>
#include <gnutls_state.h>
#include <auth_dh_common.h>

static int gen_dhe_server_kx (MHD_gtls_session_t, opaque **);
static int proc_dhe_server_kx (MHD_gtls_session_t, opaque *, size_t);
static int proc_dhe_client_kx (MHD_gtls_session_t, opaque *, size_t);

const MHD_gtls_mod_auth_st MHD_gtls_dhe_rsa_auth_struct = {
  "DHE_RSA",
  MHD_gtls_gen_cert_server_certificate,
  MHD_gtls_gen_cert_client_certificate,
  gen_dhe_server_kx,
  MHD_gtls_gen_dh_common_client_kx,
  MHD_gtls_gen_cert_client_cert_vrfy,   /* gen client cert vrfy */
  MHD_gtls_gen_cert_server_cert_req,    /* server cert request */

  MHD_gtls_proc_cert_server_certificate,
  MHD__gnutls_proc_cert_client_certificate,
  proc_dhe_server_kx,
  proc_dhe_client_kx,
  MHD_gtls_proc_cert_client_cert_vrfy,  /* proc client cert vrfy */
  MHD_gtls_proc_cert_cert_req   /* proc server cert request */
};

const MHD_gtls_mod_auth_st MHD_gtls_dhe_dss_auth_struct = {
  "DHE_DSS",
  MHD_gtls_gen_cert_server_certificate,
  MHD_gtls_gen_cert_client_certificate,
  gen_dhe_server_kx,
  MHD_gtls_gen_dh_common_client_kx,
  MHD_gtls_gen_cert_client_cert_vrfy,   /* gen client cert vrfy */
  MHD_gtls_gen_cert_server_cert_req,    /* server cert request */

  MHD_gtls_proc_cert_server_certificate,
  MHD__gnutls_proc_cert_client_certificate,
  proc_dhe_server_kx,
  proc_dhe_client_kx,
  MHD_gtls_proc_cert_client_cert_vrfy,  /* proc client cert vrfy */
  MHD_gtls_proc_cert_cert_req   /* proc server cert request */
};


static int
gen_dhe_server_kx (MHD_gtls_session_t session, opaque ** data)
{
  mpi_t g, p;
  const mpi_t *mpis;
  int ret = 0, data_size;
  MHD_gnutls_cert *apr_cert_list;
  MHD_gnutls_privkey *apr_pkey;
  int apr_cert_list_length;
  MHD_gnutls_datum_t signature, ddata;
  MHD_gtls_cert_credentials_t cred;
  MHD_gtls_dh_params_t dh_params;

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

  dh_params =
    MHD_gtls_get_dh_params (cred->dh_params, cred->params_func, session);
  mpis = MHD_gtls_dh_params_to_mpi (dh_params);
  if (mpis == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_NO_TEMPORARY_DH_PARAMS;
    }

  p = mpis[0];
  g = mpis[1];

  if ((ret = MHD_gtls_auth_info_set (session, MHD_GNUTLS_CRD_CERTIFICATE,
                                     sizeof (cert_auth_info_st), 0)) < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  MHD_gtls_dh_set_group (session, g, p);

  ret = MHD_gtls_dh_common_print_server_kx (session, g, p, data, 0);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }
  data_size = ret;

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

  MHD_gtls_write_datum16 (&(*data)[data_size], signature);
  data_size += signature.size + 2;

  MHD__gnutls_free_datum (&signature);

  return data_size;
}

static int
proc_dhe_server_kx (MHD_gtls_session_t session, opaque * data,
                    size_t _data_size)
{
  int sigsize;
  MHD_gnutls_datum_t vparams, signature;
  int ret;
  cert_auth_info_t info = MHD_gtls_get_auth_info (session);
  ssize_t data_size = _data_size;
  MHD_gnutls_cert peer_cert;

  if (info == NULL || info->ncerts == 0)
    {
      MHD_gnutls_assert ();
      /* we need this in order to get peer's certificate */
      return GNUTLS_E_INTERNAL_ERROR;
    }

  ret = MHD_gtls_proc_dh_common_server_kx (session, data, _data_size, 0);
  if (ret < 0)
    {
      MHD_gnutls_assert ();
      return ret;
    }

  /* VERIFY SIGNATURE */

  vparams.size = ret;
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
      return ret;
    }

  return ret;
}



static int
proc_dhe_client_kx (MHD_gtls_session_t session, opaque * data,
                    size_t _data_size)
{
  MHD_gtls_cert_credentials_t cred;
  int ret;
  mpi_t p, g;
  const mpi_t *mpis;
  MHD_gtls_dh_params_t dh_params;

  cred = (MHD_gtls_cert_credentials_t)
    MHD_gtls_get_cred (session->key, MHD_GNUTLS_CRD_CERTIFICATE, NULL);
  if (cred == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_INSUFFICIENT_CREDENTIALS;
    }

  dh_params =
    MHD_gtls_get_dh_params (cred->dh_params, cred->params_func, session);
  mpis = MHD_gtls_dh_params_to_mpi (dh_params);
  if (mpis == NULL)
    {
      MHD_gnutls_assert ();
      return GNUTLS_E_NO_TEMPORARY_DH_PARAMS;
    }

  p = mpis[0];
  g = mpis[1];

  ret = MHD_gtls_proc_dh_common_client_kx (session, data, _data_size, g, p);

  return ret;

}
