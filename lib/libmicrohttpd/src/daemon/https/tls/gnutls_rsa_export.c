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

/* This file contains code for RSA temporary keys. These keys are
 * only used in export cipher suites.
 */

#include <gnutls_int.h>
#include <gnutls_errors.h>
#include <gnutls_datum.h>
#include <gnutls_rsa_export.h>
#include "debug.h"
/* x509 */
#include "x509.h"
#include "privkey.h"

/* returns e and m, depends on the requested bits.
 * We only support limited key sizes.
 */
const mpi_t *
MHD__gnutls_rsa_params_to_mpi (MHD_gtls_rsa_params_t rsa_params)
{
  if (rsa_params == NULL)
    {
      return NULL;
    }
  return rsa_params->params;
}


/**
  * MHD__gnutls_rsa_params_deinit - This function will deinitialize the RSA parameters
  * @rsa_params: Is a structure that holds the parameters
  *
  * This function will deinitialize the RSA parameters structure.
  *
  **/
void
MHD__gnutls_rsa_params_deinit (MHD_gtls_rsa_params_t rsa_params)
{
  MHD_gnutls_x509_privkey_deinit (rsa_params);
}
