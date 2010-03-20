/*
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 Free Software Foundation
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

int MHD__gnutls_x509_crt_get_extension (MHD_gnutls_x509_crt_t cert,
                                        const char *extension_id, int indx,
                                        MHD_gnutls_datum_t * ret,
                                        unsigned int *critical);

int MHD__gnutls_x509_crt_get_extension_oid (MHD_gnutls_x509_crt_t cert,
                                            int indx, void *ret,
                                            size_t * ret_size);
int MHD__gnutls_x509_ext_extract_keyUsage (uint16_t * keyUsage,
                                           opaque * extnValue,
                                           int extnValueLen);
